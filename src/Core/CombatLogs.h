#pragma once

#include <wx/datetime.h>
#include <wx/string.h>

#include <string>
#include <vector>

namespace Core::CombatLogs
{
    struct CombatLogFile
    {
        wxString Path;
        wxString Name;
        wxULongLong Size = 0;
        wxDateTime Modified;
        wxDateTime Started;
        bool InUse = false;
    };

    bool IsInUse(const wxString& path);
    wxDateTime ParseTimestamp(const std::string& text, const wxDateTime& fallback);
    std::vector<CombatLogFile> Find(const wxString& logsDirectory);
}
