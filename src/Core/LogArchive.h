#pragma once

#include "Core/Expansion.h"

#include <wx/string.h>

#include <optional>

namespace Core::LogArchive
{
    struct ArchiveResult
    {
        int Moved = 0;
        int InUse = 0;
        int Failed = 0;
    };

    std::optional<wxString> PrepareArchiveDirectory(Expansion expansion, const wxString& clientFolder);
    std::optional<ArchiveResult> ArchiveAll(Expansion expansion, const wxString& clientFolder);
    int DeleteArchived(Expansion expansion, const wxString& clientFolder);
    int DeleteArchivedOlderThan(Expansion expansion, const wxString& clientFolder, int days);
}
