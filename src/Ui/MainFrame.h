#pragma once

#include "Core/Settings.h"
#include "Core/Api.h"
#include "Ui/FolderPromptView.h"
#include "Ui/LoginView.h"
#include "Ui/LogsView.h"
#include "Ui/NavBar.h"
#include "Ui/SettingsView.h"
#include "Ui/SubView.h"
#include "Ui/TrayIcon.h"
#include "Ui/UploadView.h"

#include <wx/frame.h>
#include <wx/simplebook.h>
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
            PAGE_SETTINGS,
            PAGE_LOGIN,
            PAGE_UPLOAD
        };

        void ShowCurrentView();
        void OnExpansionSelected(Core::Expansion expansion);
        void OnSubViewSelected(SubView subView);
        void OnRootFolderChosen(const wxString& folder);
        void OnExpansionFolderChosen(const wxString& folder);
        void OnClose(wxCloseEvent& event);
        void RunArchiveCleanup();
        void OnArchiveQueueTimer();
        void UpdateArchiveQueueTimer();
        void ArchiveLog(const wxString& path);
        bool ProcessArchiveQueue();
        void OnLoggedIn(const wxString& token);
        void OnLogout();
        void LoadProfile();
        void BeginUpload(const wxString& path);
        void OnUploaded(const std::vector<wxString>& paths);

        Core::Settings& _settings;
        Core::Expansion _expansion;
        SubView _subView = SubView::Logs;
        wxString _token;
        std::vector<Core::Api::Guild> _guilds;
        std::vector<Core::Api::ExpansionInfo> _expansions;
        bool _showUpload = false;

        NavBar* _navBar = nullptr;
        wxWindow* _navLine = nullptr;
        wxSimplebook* _book = nullptr;
        FolderPromptView* _rootPrompt = nullptr;
        FolderPromptView* _expansionPrompt = nullptr;
        LogsView* _logsView = nullptr;
        SettingsView* _settingsView = nullptr;
        LoginView* _loginView = nullptr;
        UploadView* _uploadView = nullptr;
        TrayIcon* _trayIcon = nullptr;
        wxTimer _cleanupTimer;
        wxTimer _archiveQueueTimer;
    };
}
