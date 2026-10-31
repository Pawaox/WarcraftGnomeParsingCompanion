#pragma once

#include <wx/string.h>

namespace Core::AppPaths
{
    wxString ExePath();
    wxString ExeDirectory();
    wxString SettingsFile();
    wxString ArchiveFallbackDirectory();
    bool IsDirectoryWritable(const wxString& directory);
}
