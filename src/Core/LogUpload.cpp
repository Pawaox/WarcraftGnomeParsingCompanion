#include "Core/LogUpload.h"

#include "Core/Api.h"
#include "Core/Checksum.h"

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/zstream.h>

#include <algorithm>
#include <chrono>
#include <set>
#include <thread>

namespace Core::LogUpload
{
    namespace
    {
        using LogScanner::LogPiece;

        constexpr size_t RAW_SLICE_BYTES = 32 * 1024 * 1024;
        constexpr size_t READ_BYTES = 1024 * 1024;
        constexpr int STATUS_POLL_SECONDS = 3;
        constexpr int STATUS_POLL_LIMIT = 400;

        const wxString FILE_CHANGED = "A log changed after it was read. Go back and read it again.";

        class ChunkSender
        {
        public:
            ChunkSender(const UploadRequest& request, const wxString& uploadKey)
                : _request(request),
                  _uploadKey(uploadKey)
            {
                _slice.reserve(RAW_SLICE_BYTES);
            }

            bool Add(const char* data, size_t size)
            {
                while (size > 0)
                {
                    size_t take = std::min(size, RAW_SLICE_BYTES - _slice.size());
                    _slice.insert(_slice.end(), data, data + take);
                    data += take;
                    size -= take;

                    if (_slice.size() == RAW_SLICE_BYTES && !Flush())
                        return false;
                }
                return true;
            }

            bool Flush()
            {
                if (_slice.empty())
                    return true;

                wxMemoryOutputStream compressed;
                {
                    wxZlibOutputStream gzip(compressed, wxZ_DEFAULT_COMPRESSION, wxZLIB_GZIP);
                    gzip.Write(_slice.data(), _slice.size());
                    gzip.Close();
                }

                Api::Response response = Api::PostChunk(_request.Token, _uploadKey, _chunks,
                    compressed.GetOutputStreamBuffer()->GetBufferStart(), compressed.GetSize());
                if (!response.Ok)
                {
                    _error = response.Error;
                    return false;
                }

                ++_chunks;
                _slice.clear();
                return true;
            }

            int Chunks() const { return _chunks; }
            const wxString& Error() const { return _error; }

        private:
            const UploadRequest& _request;
            wxString _uploadKey;
            std::vector<char> _slice;
            wxString _error;
            int _chunks = 0;
        };

        std::vector<LogPiece> SelectedPieces(const UploadRequest& request)
        {
            std::vector<LogPiece> pieces;
            for (size_t index : request.SelectedGroups)
            {
                const LogScanner::LogGroup& group = request.Scan.Groups[index];
                pieces.insert(pieces.end(), group.Pieces.begin(), group.Pieces.end());
            }

            std::ranges::sort(pieces, [](const LogPiece& left, const LogPiece& right)
            {
                return left.FileIndex != right.FileIndex ? left.FileIndex < right.FileIndex : left.Start < right.Start;
            });
            return pieces;
        }

        bool FilesUnchanged(const UploadRequest& request)
        {
            for (const LogScanner::LogFile& file : request.Scan.Files)
            {
                if (wxFileName(file.Path).GetSize().GetValue() < file.Size)
                    return false;
            }
            return true;
        }

        bool SendPiece(wxFFile& input, const LogPiece& piece, ChunkSender& sender, uint64_t& sentBytes, uint64_t totalBytes,
            const std::function<void(const wxString&, int)>& onProgress, const std::atomic<bool>& cancelled, wxString& error)
        {
            if (!input.Seek(static_cast<wxFileOffset>(piece.Start)))
            {
                error = FILE_CHANGED;
                return false;
            }

            std::vector<char> buffer(READ_BYTES);
            uint64_t remaining = piece.End - piece.Start;
            uint32_t checksum = 0;

            while (remaining > 0)
            {
                if (cancelled)
                {
                    error = "Cancelled.";
                    return false;
                }

                size_t want = static_cast<size_t>(std::min<uint64_t>(remaining, buffer.size()));
                if (input.Read(buffer.data(), want) != want)
                {
                    error = FILE_CHANGED;
                    return false;
                }

                checksum = Checksum::Update(checksum, buffer.data(), want);
                if (!sender.Add(buffer.data(), want))
                {
                    error = sender.Error();
                    return false;
                }

                remaining -= want;
                sentBytes += want;
                onProgress("Uploading", static_cast<int>(sentBytes * 100 / totalBytes));
            }

            if (checksum != piece.Checksum)
            {
                error = FILE_CHANGED;
                return false;
            }
            return true;
        }

        UploadResult WaitForImport(const wxString& token, long long workItemID,
            const std::function<void(const wxString&, int)>& onProgress, const std::atomic<bool>& cancelled)
        {
            for (int poll = 0; poll < STATUS_POLL_LIMIT && !cancelled; ++poll)
            {
                std::this_thread::sleep_for(std::chrono::seconds(STATUS_POLL_SECONDS));

                Api::Response response = Api::Post("/api/companion/upload/status", { { "workItemID", workItemID } }, token);
                if (!response.Ok || !response.Result.is_object())
                    continue;

                if (!response.Result.value("failedOn", nlohmann::json()).is_null())
                    return { false, Api::FromJson(response.Result.value("resultMessage", nlohmann::json())) };
                if (!response.Result.value("finishedOn", nlohmann::json()).is_null())
                    return { true, "Uploaded. The log is on the website now." };

                onProgress("The website is reading the log", response.Result.value("progressPercent", 0));
            }
            return { true, "Uploaded. The website is still reading the log." };
        }
    }

    UploadResult Run(const UploadRequest& request, const std::function<void(const wxString&, int)>& onProgress, const std::atomic<bool>& cancelled)
    {
        wxLogNull noLog;

        if (!FilesUnchanged(request))
            return { false, FILE_CHANGED };

        std::vector<LogPiece> pieces = SelectedPieces(request);
        uint64_t totalBytes = 0;
        for (const LogPiece& piece : pieces)
            totalBytes += piece.End - piece.Start;
        if (totalBytes == 0)
            return { false, "Nothing selected." };

        const LogScanner::LogFile& firstFile = request.Scan.Files[pieces.front().FileIndex];
        nlohmann::json begin =
        {
            { "guildID", request.GuildID },
            { "expansionID", request.ExpansionID },
            { "name", Api::ToJson(request.Name) },
            { "warcraftLogsID", Api::ToJson(request.WarcraftLogsID) },
            { "fileName", Api::ToJson(wxFileName(firstFile.Path).GetFullName()) },
            { "isPublic", request.IsPublic },
            { "fileModifiedEpochMs", firstFile.Modified.IsValid() ? firstFile.Modified.GetValue().GetValue() : 0LL }
        };

        onProgress("Starting upload", 0);
        Api::Response started = Api::Post("/api/companion/upload/begin", begin, request.Token);
        if (!started.Ok)
            return { false, started.Error };

        wxString uploadKey = Api::FromJson(started.Result);
        ChunkSender sender(request, uploadKey);
        std::set<size_t> headerWritten;
        uint64_t sentBytes = 0;
        wxString error;
        wxFFile input;
        size_t openFile = pieces.front().FileIndex + 1;

        for (const LogPiece& piece : pieces)
        {
            if (piece.FileIndex != openFile)
            {
                openFile = piece.FileIndex;
                if (!input.Open(request.Scan.Files[openFile].Path, "rb"))
                    return { false, FILE_CHANGED };
            }

            if (headerWritten.insert(piece.FileIndex).second && piece.Start > 0)
            {
                const std::string& header = request.Scan.Files[piece.FileIndex].Header;
                if (!sender.Add(header.data(), header.size()))
                    return { false, sender.Error() };
            }

            if (!SendPiece(input, piece, sender, sentBytes, totalBytes, onProgress, cancelled, error))
                return { false, error };
        }

        if (!sender.Flush())
            return { false, sender.Error() };

        onProgress("Finishing upload", 100);
        nlohmann::json complete = { { "uploadKey", Api::ToJson(uploadKey) }, { "chunkCount", sender.Chunks() } };
        Api::Response finished = Api::Post("/api/companion/upload/complete", complete, request.Token);
        if (!finished.Ok)
            return { false, finished.Error };

        return WaitForImport(request.Token, finished.Result.get<long long>(), onProgress, cancelled);
    }
}
