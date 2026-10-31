#pragma once

#include "Core/Settings.h"
#include "Ui/FolderPromptView.h"
#include "Ui/LogsView.h"
#include "Ui/NavBar.h"
#include "Ui/SettingsView.h"
#include "Ui/SubView.h"
#include "Ui/TrayIcon.h"

#include <wx/frame.h>
#include <wx/simplebook.h>
#include <wx/statline.h>
#include <wx/timer.h>

namespace Ui
{
    class MainFrame : public wxFrame
    {
    public:
        explicit MainFrame(Core::Settings& settings);
        ~MainFrame() override;

        void ShowFromTray();

    private:
        enum Page
        {
            PAGE_ROOT_PROMPT,
            PAGE_EXPANSION_PROMPT,
            PAGE_LOGS,
            PAGE_SETTINGS
        };

        void ShowCurrentView();
        void OnExpansionSelected(Core::Expansion expansion);
        void OnSubViewSelected(SubView subView);
        void OnRootFolderChosen(const wxString& folder);
        void OnExpansionFolderChosen(const wxString& folder);
        void OnClose(wxCloseEvent& event);
        void RunArchiveCleanup();

        Core::Settings& _settings;
        Core::Expansion _expansion;
        SubView _subView = SubView::Logs;

        NavBar* _navBar = nullptr;
        wxStaticLine* _navLine = nullptr;
        wxSimplebook* _book = nullptr;
        FolderPromptView* _rootPrompt = nullptr;
        FolderPromptView* _expansionPrompt = nullptr;
        LogsView* _logsView = nullptr;
        SettingsView* _settingsView = nullptr;
        TrayIcon* _trayIcon = nullptr;
        wxTimer _cleanupTimer;
    };
}
