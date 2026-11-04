#include "Core/LogScanner.h"

#include "Core/Checksum.h"
#include "Core/CombatLogs.h"

#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/log.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string_view>

namespace Core::LogScanner
{
    namespace
    {
        constexpr size_t BUFFER_BYTES = 4 * 1024 * 1024;
        constexpr size_t TIMESTAMP_SEARCH = 40;
        constexpr int FOLD_MINUTES = 5;
        constexpr unsigned long AFFILIATION_MASK = 0xF;
        constexpr unsigned long AFFILIATION_MINE = 0x1;
        constexpr unsigned long TYPE_PLAYER = 0x400;

        constexpr std::string_view ZONE_CHANGE = "ZONE_CHANGE,";
        constexpr std::string_view ENCOUNTER_END = "ENCOUNTER_END,";
        constexpr std::string_view PLAYER_PREFIX = "Player-";

        struct Segment
        {
            LogPiece Piece;
            wxString Zone;
            std::string Guid;
            wxString Character;
            std::string FirstTimestamp;
            std::string LastTimestamp;
            int Kills = 0;
            int Wipes = 0;
        };

        std::string_view Field(std::string_view& text)
        {
            size_t comma = text.find(',');
            std::string_view field = text.substr(0, comma);
            text = comma == std::string_view::npos ? std::string_view() : text.substr(comma + 1);
            return field;
        }

        std::string_view Unquote(std::string_view text)
        {
            if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
                return text.substr(1, text.size() - 2);
            return text;
        }

        wxString ReadZone(std::string_view rest)
        {
            Field(rest);
            size_t open = rest.find('"');
            size_t close = rest.find('"', open + 1);
            if (open == std::string_view::npos || close == std::string_view::npos)
                return wxEmptyString;
            return wxString::FromUTF8(rest.data() + open + 1, close - open - 1);
        }

        bool ReadEncounterKill(std::string_view rest)
        {
            size_t close = rest.rfind('"');
            std::string_view tail = rest.substr(close + 2);
            Field(tail);
            Field(tail);
            return !tail.empty() && tail.front() == '1';
        }

        bool ReadOwner(std::string_view event, std::string& guid, wxString& character)
        {
            size_t comma = event.find(',');
            if (comma == std::string_view::npos)
                return false;

            std::string_view rest = event.substr(comma + 1);
            if (rest.substr(0, PLAYER_PREFIX.size()) != PLAYER_PREFIX)
                return false;

            std::string_view id = Field(rest);
            std::string_view quotedName = Field(rest);
            if (quotedName.empty() || quotedName.front() != '"')
                return false;

            std::string_view name = Unquote(quotedName);
            std::string_view flagsText = rest.substr(0, rest.find_first_of(",\r\n"));

            std::string flagsCopy(flagsText);
            unsigned long flags = std::strtoul(flagsCopy.c_str(), nullptr, 16);
            if ((flags & AFFILIATION_MASK) != AFFILIATION_MINE || (flags & TYPE_PLAYER) == 0)
                return false;

            guid = id;
            character = wxString::FromUTF8(name.data(), name.size()).BeforeFirst('-');
            return true;
        }

        wxDateTime ParseTime(const std::string& timestamp, const wxDateTime& fallback)
        {
            wxDateTime parsed = CombatLogs::ParseTimestamp(timestamp, fallback);
            return parsed.IsValid() ? parsed : fallback;
        }

        std::string SegmentKey(const Segment& segment)
        {
            return std::string(segment.Zone.utf8_string()) + "\n" + segment.Guid;
        }

        void FillMissingOwners(std::vector<Segment>& segments)
        {
            for (size_t index = 1; index < segments.size(); ++index)
            {
                if (segments[index].Guid.empty())
                {
                    segments[index].Guid = segments[index - 1].Guid;
                    segments[index].Character = segments[index - 1].Character;
                }
            }

            for (size_t index = segments.size(); index-- > 1;)
            {
                if (segments[index - 1].Guid.empty())
                {
                    segments[index - 1].Guid = segments[index].Guid;
                    segments[index - 1].Character = segments[index].Character;
                }
            }
        }

        void FoldShortTrips(std::vector<Segment>& segments, const std::vector<LogFile>& files)
        {
            for (size_t index = 1; index + 1 < segments.size(); ++index)
            {
                Segment& trip = segments[index];
                const Segment& before = segments[index - 1];
                const Segment& after = segments[index + 1];
                if (trip.Kills > 0 || trip.Wipes > 0 || SegmentKey(before) != SegmentKey(after) || SegmentKey(trip) == SegmentKey(before))
                    continue;

                const wxDateTime& fallback = files[trip.Piece.FileIndex].Modified;
                wxDateTime started = ParseTime(trip.FirstTimestamp, fallback);
                wxDateTime ended = ParseTime(trip.LastTimestamp, fallback);
                if (ended - started <= wxTimeSpan::Minutes(FOLD_MINUTES))
                {
                    trip.Zone = before.Zone;
                    trip.Guid = before.Guid;
                    trip.Character = before.Character;
                }
            }
        }

        std::vector<LogGroup> BuildGroups(const std::vector<Segment>& segments, const std::vector<LogFile>& files)
        {
            std::vector<LogGroup> groups;
            std::map<std::string, size_t> indexes;

            for (const Segment& segment : segments)
            {
                std::string key = SegmentKey(segment);
                auto found = indexes.find(key);
                if (found == indexes.end())
                {
                    found = indexes.emplace(key, groups.size()).first;
                    LogGroup group;
                    group.Zone = segment.Zone.empty() ? wxString("Unknown zone") : segment.Zone;
                    group.Character = segment.Character;
                    groups.push_back(group);
                }

                LogGroup& group = groups[found->second];
                const wxDateTime& fallback = files[segment.Piece.FileIndex].Modified;
                wxDateTime started = ParseTime(segment.FirstTimestamp, fallback);
                wxDateTime ended = ParseTime(segment.LastTimestamp, fallback);
                if (!group.Started.IsValid() || started < group.Started)
                    group.Started = started;
                if (!group.Ended.IsValid() || ended > group.Ended)
                    group.Ended = ended;

                group.Kills += segment.Kills;
                group.Wipes += segment.Wipes;
                group.Bytes += segment.Piece.End - segment.Piece.Start;
                group.Pieces.push_back(segment.Piece);
            }
            return groups;
        }

        class FileScanner
        {
        public:
            FileScanner(size_t fileIndex, LogFile& file, std::vector<Segment>& segments)
                : _file(file),
                  _segments(segments)
            {
                _current.Piece.FileIndex = fileIndex;
            }

            void Line(const char* data, size_t size)
            {
                std::string_view line(data, size);
                size_t separator = line.substr(0, TIMESTAMP_SEARCH).find("  ");

                if (separator != std::string_view::npos)
                {
                    std::string_view event = line.substr(separator + 2);
                    Inspect(line, separator, event);
                    _lastTimestamp.assign(data, separator);
                    if (_current.FirstTimestamp.empty())
                        _current.FirstTimestamp = _lastTimestamp;
                    _current.LastTimestamp = _lastTimestamp;
                }

                if (_file.Header.empty())
                    _file.Header.assign(data, size);

                _current.Piece.Checksum = Checksum::Update(_current.Piece.Checksum, data, size);
                _offset += size;
            }

            void Finish()
            {
                Close();
            }

        private:
            void Inspect(std::string_view line, size_t separator, std::string_view event)
            {
                if (event.substr(0, ZONE_CHANGE.size()) == ZONE_CHANGE)
                {
                    wxString zone = ReadZone(event.substr(ZONE_CHANGE.size()));
                    if (!_current.Zone.empty())
                        Close();
                    _current.Zone = zone;
                }
                else if (event.substr(0, ENCOUNTER_END.size()) == ENCOUNTER_END)
                {
                    if (ReadEncounterKill(event.substr(ENCOUNTER_END.size())))
                        ++_current.Kills;
                    else
                        ++_current.Wipes;
                }
                else if (std::string guid; ReadOwner(event, guid, _owner))
                {
                    if (!_current.Guid.empty() && _current.Guid != guid)
                    {
                        wxString zone = _current.Zone;
                        Close();
                        _current.Zone = zone;
                    }
                    if (_current.Guid.empty())
                    {
                        _current.Guid = guid;
                        _current.Character = _owner;
                    }
                }
            }

            void Close()
            {
                _current.Piece.Start = _pieceStart;
                _current.Piece.End = _offset;
                if (_current.Piece.End > _current.Piece.Start)
                    _segments.push_back(_current);

                size_t fileIndex = _current.Piece.FileIndex;
                _current = Segment();
                _current.Piece.FileIndex = fileIndex;
                _pieceStart = _offset;
            }

            LogFile& _file;
            std::vector<Segment>& _segments;
            Segment _current;
            std::string _lastTimestamp;
            wxString _owner;
            uint64_t _offset = 0;
            uint64_t _pieceStart = 0;
        };

        bool ScanFile(size_t fileIndex, LogFile& file, std::vector<Segment>& segments, uint64_t& doneBytes, uint64_t totalBytes,
            const std::function<void(int)>& onProgress, const std::atomic<bool>& cancelled)
        {
            wxFFile input(file.Path, "rb");
            if (!input.IsOpened())
                return false;

            FileScanner scanner(fileIndex, file, segments);
            std::vector<char> buffer(BUFFER_BYTES);
            size_t filled = 0;

            while (!cancelled)
            {
                size_t read = input.Read(buffer.data() + filled, buffer.size() - filled);
                if (read == 0)
                    break;

                filled += read;
                doneBytes += read;

                size_t lineStart = 0;
                while (lineStart < filled)
                {
                    const void* newline = std::memchr(buffer.data() + lineStart, '\n', filled - lineStart);
                    if (!newline)
                        break;

                    size_t lineEnd = static_cast<const char*>(newline) - buffer.data() + 1;
                    scanner.Line(buffer.data() + lineStart, lineEnd - lineStart);
                    lineStart = lineEnd;
                }

                filled -= lineStart;
                std::memmove(buffer.data(), buffer.data() + lineStart, filled);

                if (totalBytes > 0)
                    onProgress(static_cast<int>(doneBytes * 100 / totalBytes));
            }

            scanner.Finish();
            return true;
        }
    }

    ScanResult Scan(const std::vector<wxString>& paths, const std::function<void(int)>& onProgress, const std::atomic<bool>& cancelled)
    {
        wxLogNull noLog;
        ScanResult result;
        uint64_t totalBytes = 0;

        for (const wxString& path : paths)
        {
            LogFile file;
            wxFileName name(path);
            file.Path = path;
            file.Size = name.GetSize().GetValue();
            file.Modified = name.GetModificationTime();
            totalBytes += file.Size;
            result.Files.push_back(file);
        }

        std::vector<Segment> segments;
        uint64_t doneBytes = 0;
        for (size_t index = 0; index < result.Files.size(); ++index)
        {
            if (!ScanFile(index, result.Files[index], segments, doneBytes, totalBytes, onProgress, cancelled))
            {
                result.Error = "Couldn't read " + wxFileName(result.Files[index].Path).GetFullName();
                return result;
            }
        }

        FillMissingOwners(segments);
        FoldShortTrips(segments, result.Files);
        result.Groups = BuildGroups(segments, result.Files);
        return result;
    }
}
