#pragma once

#include "Core/Expansion.h"
#include "Core/ThemeMode.h"

#include <wx/string.h>

#include <array>

namespace Core
{
    inline constexpr std::array<int, 6> AUTO_DELETE_ARCHIVE_DAYS = { 0, 3, 7, 14, 30, 60 };

    class Settings
    {
    public:
        static Settings Load();
        void Save() const;

        wxString& ExpansionFolder(Expansion expansion);
        const wxString& ExpansionFolder(Expansion expansion) const;

        wxString WowRoot;
        bool AutoArchive = false;
        int AutoDeleteArchiveDays = 0;
        bool RunOnStartup = false;
        bool CloseToTray = false;
        ThemeMode Theme = ThemeMode::System;
        Expansion LastExpansion = DEFAULT_EXPANSION;

    private:
        std::array<wxString, ALL_EXPANSIONS.size()> _expansionFolders;
    };
}
