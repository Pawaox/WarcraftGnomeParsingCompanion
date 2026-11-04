#include "Core/Updater.h"

#include "Core/AppInfo.h"
#include "Core/AppPaths.h"

#include <nlohmann/json.hpp>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/log.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>
#include <wx/webrequest.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#include <bcrypt.h>
#endif

#include <algorithm>
#include <array>
#include <vector>

namespace Core::Updater
{
    namespace
    {
        const wxString MANIFEST_FILE = "version.json";
        const wxString EXE_FILE = "GnomeparsingCompanion.exe";
        const wxString NEW_SUFFIX = ".new";
        const wxString OLD_SUFFIX = ".old";
        constexpr int REMOVE_ATTEMPTS = 20;
        constexpr int REMOVE_RETRY_MS = 100;

        wxString baseUrl = AppInfo::UPDATE_URL;

        wxString NewExePath()
        {
            return AppPaths::ExePath() + NEW_SUFFIX;
        }

        wxString OldExePath()
        {
            return AppPaths::ExePath() + OLD_SUFFIX;
        }

        std::vector<long> VersionParts(const wxString& version)
        {
            std::vector<long> parts;
            for (const wxString& part : wxSplit(version, '.'))
            {
                long number = 0;
                part.ToLong(&number);
                parts.push_back(number);
            }
            return parts;
        }

#ifdef __WXMSW__
        wxString Sha256(const std::vector<char>& data)
        {
            std::array<UCHAR, 32> hash{};
            NTSTATUS status = BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
                reinterpret_cast<PUCHAR>(const_cast<char*>(data.data())), static_cast<ULONG>(data.size()),
                hash.data(), static_cast<ULONG>(hash.size()));
            if (!BCRYPT_SUCCESS(status))
                return wxEmptyString;

            wxString text;
            for (UCHAR value : hash)
                text += wxString::Format("%02x", value);
            return text;
        }
#endif
    }

    void SetBaseUrl(const wxString& url)
    {
        baseUrl = url;
    }

    bool IsOlder(const wxString& version, const wxString& than)
    {
        std::vector<long> left = VersionParts(version);
        std::vector<long> right = VersionParts(than);
        size_t length = std::max(left.size(), right.size());
        left.resize(length);
        right.resize(length);
        return left < right;
    }

    std::optional<Release> Check()
    {
#ifdef __WXMSW__
        wxWebRequestSync request = wxWebSessionSync::GetDefault().CreateRequest(baseUrl + MANIFEST_FILE);
        wxWebRequestSync::Result result = request.Execute();
        wxWebResponse response = request.GetResponse();
        if (!result || !response.IsOk() || response.GetStatus() != 200)
            return std::nullopt;

        nlohmann::json json = nlohmann::json::parse(response.AsString().utf8_string(), nullptr, false);
        if (json.is_discarded() || !json.is_object())
            return std::nullopt;

        Release release;
        release.Latest = wxString::FromUTF8(json.value("latest", std::string()));
        release.Minimum = wxString::FromUTF8(json.value("minimum", std::string()));
        release.Sha256 = wxString::FromUTF8(json.value("sha256", std::string())).Lower();
        if (release.Latest.empty() || release.Sha256.empty())
            return std::nullopt;

        return release;
#else
        return std::nullopt;
#endif
    }

    wxString Download(const Release& release)
    {
#ifdef __WXMSW__
        wxWebRequestSync request = wxWebSessionSync::GetDefault().CreateRequest(baseUrl + EXE_FILE);
        wxWebRequestSync::Result result = request.Execute();
        wxWebResponse response = request.GetResponse();
        if (!result || !response.IsOk() || response.GetStatus() != 200)
            return "Couldn't download it.";

        wxInputStream* stream = response.GetStream();
        if (!stream)
            return "Couldn't download it.";

        std::vector<char> data(static_cast<size_t>(stream->GetLength()));
        stream->Read(data.data(), data.size());
        if (stream->LastRead() != data.size())
            return "Couldn't download it.";

        if (Sha256(data) != release.Sha256)
            return "The download was damaged.";

        wxLogNull noLog;
        wxFFile file(NewExePath(), "wb");
        if (!file.IsOpened() || file.Write(data.data(), data.size()) != data.size() || !file.Close())
            return "Couldn't save it.";

        return wxEmptyString;
#else
        return "Updating isn't supported here.";
#endif
    }

    wxString Install()
    {
#ifdef __WXMSW__
        wxLogNull noLog;
        wxString exe = AppPaths::ExePath();

        wxRemoveFile(OldExePath());
        if (!wxRenameFile(exe, OldExePath()))
            return "Couldn't replace the app.";

        if (!wxRenameFile(NewExePath(), exe))
        {
            wxRenameFile(OldExePath(), exe);
            return "Couldn't replace the app.";
        }

        if (wxExecute("\"" + exe + "\" " + AppInfo::UPDATED_ARGUMENT, wxEXEC_ASYNC) == 0)
        {
            wxRenameFile(exe, NewExePath());
            wxRenameFile(OldExePath(), exe);
            return "Couldn't start the new version.";
        }

        return wxEmptyString;
#else
        return "Updating isn't supported here.";
#endif
    }

    void RemoveOldVersion()
    {
        wxLogNull noLog;
        wxRemoveFile(NewExePath());

        for (int attempt = 0; attempt < REMOVE_ATTEMPTS && wxFileExists(OldExePath()); ++attempt)
        {
            if (!wxRemoveFile(OldExePath()))
                wxMilliSleep(REMOVE_RETRY_MS);
        }
    }
}
