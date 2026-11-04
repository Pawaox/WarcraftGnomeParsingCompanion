#pragma once

#include "Core/Expansion.h"
#include "Core/ThemeMode.h"

#include <wx/string.h>

#include <array>
#include <vector>

namespace Core
{
    inline constexpr std::array<int, 6> AUTO_DELETE_ARCHIVE_DAYS = { 0, 3, 7, 14, 30, 60 };

    struct QueuedArchive
    {
        Expansion LogExpansion;
        wxString Path;
    };

    class Settings
    {
    public:
        static Settings Load();
        void Save() const;

        wxString& ExpansionFolder(Expansion expansion);
        const wxString& ExpansionFolder(Expansion expansion) const;
        bool IsArchiveQueued(const wxString& path) const;

        wxString WowRoot;
        bool AutoArchive = false;
        int AutoDeleteArchiveDays = 0;
        bool RunOnStartup = false;
        bool CloseToTray = false;
        ThemeMode Theme = ThemeMode::System;
        Expansion LastExpansion = DEFAULT_EXPANSION;
        bool AutoRefresh = true;
        wxString WebsiteUrl;
        wxString UpdateUrl;
        std::vector<QueuedArchive> ArchiveQueue;

    private:
        std::array<wxString, ALL_EXPANSIONS.size()> _expansionFolders;
    };
}
