#include "Core/Settings.h"

#include "Core/AppPaths.h"

#include <wx/fileconf.h>

#include <algorithm>
#include <memory>

namespace Core
{
    namespace
    {
        constexpr wxChar QUEUE_SEPARATOR = '|';

        std::unique_ptr<wxFileConfig> OpenConfig()
        {
            return std::make_unique<wxFileConfig>(wxEmptyString, wxEmptyString, AppPaths::SettingsFile(),
                wxEmptyString, wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_NO_ESCAPE_CHARACTERS);
        }

        wxString ThemeToString(ThemeMode theme)
        {
            switch (theme)
            {
                case ThemeMode::Light: return "Light";
                case ThemeMode::Dark: return "Dark";
                case ThemeMode::System: break;
            }
            return "System";
        }

        ThemeMode ThemeFromString(const wxString& value)
        {
            if (value.IsSameAs("Light", false))
                return ThemeMode::Light;
            if (value.IsSameAs("Dark", false))
                return ThemeMode::Dark;
            return ThemeMode::System;
        }

        wxString FolderKey(Expansion expansion)
        {
            return "Wow/" + ExpansionKey(expansion);
        }

        wxString QueueKey(size_t index)
        {
            return wxString::Format("ArchiveQueue/%zu", index);
        }
    }

    Settings Settings::Load()
    {
        std::unique_ptr<wxFileConfig> config = OpenConfig();
        Settings settings;

        config->Read("Wow/Root", &settings.WowRoot);
        for (Expansion expansion : ALL_EXPANSIONS)
            config->Read(FolderKey(expansion), &settings.ExpansionFolder(expansion));

        config->Read("Cleanup/AutoArchive", &settings.AutoArchive, false);
        config->Read("Cleanup/AutoDeleteArchiveDays", &settings.AutoDeleteArchiveDays, 0);
        if (std::ranges::find(AUTO_DELETE_ARCHIVE_DAYS, settings.AutoDeleteArchiveDays) == AUTO_DELETE_ARCHIVE_DAYS.end())
            settings.AutoDeleteArchiveDays = 0;

        config->Read("Startup/RunOnStartup", &settings.RunOnStartup, false);
        config->Read("Startup/CloseToTray", &settings.CloseToTray, false);

        settings.Theme = ThemeFromString(config->Read("UI/Theme", wxEmptyString));
        settings.LastExpansion = ExpansionFromKey(config->Read("UI/LastExpansion", wxEmptyString)).value_or(DEFAULT_EXPANSION);

        config->Read("UI/AutoRefresh", &settings.AutoRefresh, true);
        config->Read("Website/Url", &settings.WebsiteUrl);

        wxString entry;
        for (size_t index = 0; config->Read(QueueKey(index), &entry); ++index)
        {
            std::optional<Expansion> expansion = ExpansionFromKey(entry.BeforeFirst(QUEUE_SEPARATOR));
            if (expansion)
                settings.ArchiveQueue.push_back({ *expansion, entry.AfterFirst(QUEUE_SEPARATOR) });
        }

        return settings;
    }

    void Settings::Save() const
    {
        std::unique_ptr<wxFileConfig> config = OpenConfig();

        config->Write("Wow/Root", WowRoot);
        for (Expansion expansion : ALL_EXPANSIONS)
            config->Write(FolderKey(expansion), ExpansionFolder(expansion));

        config->Write("Cleanup/AutoArchive", AutoArchive);
        config->Write("Cleanup/AutoDeleteArchiveDays", AutoDeleteArchiveDays);
        config->Write("Startup/RunOnStartup", RunOnStartup);
        config->Write("Startup/CloseToTray", CloseToTray);
        config->Write("UI/Theme", ThemeToString(Theme));
        config->Write("UI/LastExpansion", ExpansionKey(LastExpansion));
        config->Write("UI/AutoRefresh", AutoRefresh);

        config->DeleteGroup("ArchiveQueue");
        for (size_t index = 0; index < ArchiveQueue.size(); ++index)
            config->Write(QueueKey(index), ExpansionKey(ArchiveQueue[index].LogExpansion) + QUEUE_SEPARATOR + ArchiveQueue[index].Path);

        config->Flush();
    }

    wxString& Settings::ExpansionFolder(Expansion expansion)
    {
        return _expansionFolders[ExpansionIndex(expansion)];
    }

    const wxString& Settings::ExpansionFolder(Expansion expansion) const
    {
        return _expansionFolders[ExpansionIndex(expansion)];
    }

    bool Settings::IsArchiveQueued(const wxString& path) const
    {
        return std::ranges::any_of(ArchiveQueue, [&path](const QueuedArchive& queued) { return queued.Path == path; });
    }
}
