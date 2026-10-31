#pragma once

#include "Core/Settings.h"

#include <wx/string.h>

namespace Core::WowFolders
{
    bool LooksLikeWowRoot(const wxString& root);
    void ChangeRoot(Settings& settings, const wxString& root);
    wxString LogsDirectory(const wxString& clientFolder);
}
