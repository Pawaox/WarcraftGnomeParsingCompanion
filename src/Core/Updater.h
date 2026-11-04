#pragma once

#include <wx/string.h>

#include <optional>

namespace Core::Updater
{
    struct Release
    {
        wxString Latest;
        wxString Minimum;
        wxString Sha256;
    };

    void SetBaseUrl(const wxString& baseUrl);
    bool IsOlder(const wxString& version, const wxString& than);

    std::optional<Release> Check();
    wxString Download(const Release& release);
    wxString Install();
    void RemoveOldVersion();
}
