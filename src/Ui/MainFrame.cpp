#include "Ui/MainFrame.h"

#include "Core/AppInfo.h"
#include "Core/LogArchive.h"
#include "Core/WowFolders.h"
#include "Ui/Images.h"

#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/taskbar.h>

namespace Ui
{
    namespace
    {
        constexpr int CLEANUP_INTERVAL_MS = 60 * 60 * 1000;

        wxString ClientFolderHint(Core::Expansion expansion)
        {
            std::vector<wxString> clientFolders = Core::ExpansionClientFolders(expansion);
            if (clientFolders.empty())
                return wxEmptyString;
            return " It's usually the " + clientFolders.front() + " folder.";
        }
    }

    MainFrame::MainFrame(Core::Settings& settings)
        : wxFrame(nullptr, wxID_ANY, Core::AppInfo::NAME),
          _settings(settings),
          _expansion(settings.LastExpansion)
    {
        SetIcons(Images::AppIcons());

        auto* root = new wxPanel(this);
        _navBar = new NavBar(root,
            [this](Core::Expansion expansion) { OnExpansionSelected(expansion); },
            [this](SubView subView) { OnSubViewSelected(subView); });
        _navLine = new wxStaticLine(root);

        _book = new wxSimplebook(root);
        _rootPrompt = new FolderPromptView(_book, [this](const wxString& folder) { OnRootFolderChosen(folder); });
        _expansionPrompt = new FolderPromptView(_book, [this](const wxString& folder) { OnExpansionFolderChosen(folder); });
        _logsView = new LogsView(_book);
        _settingsView = new SettingsView(_book, _settings,
            [this] { return _expansion; },
            [this] { ShowCurrentView(); });
        _book->AddPage(_rootPrompt, wxEmptyString);
        _book->AddPage(_expansionPrompt, wxEmptyString);
        _book->AddPage(_logsView, wxEmptyString);
        _book->AddPage(_settingsView, wxEmptyString);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(_navBar, 0, wxEXPAND);
        sizer->Add(_navLine, 0, wxEXPAND);
        sizer->Add(_book, 1, wxEXPAND);
        root->SetSizer(sizer);

        SetMinSize(FromDIP(wxSize(640, 480)));
        SetSize(FromDIP(wxSize(900, 640)));
        Centre();

        if (wxTaskBarIcon::IsAvailable())
            _trayIcon = new TrayIcon([this] { ShowFromTray(); }, [this] { Close(true); });

        Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);
        _cleanupTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { RunArchiveCleanup(); });
        _cleanupTimer.Start(CLEANUP_INTERVAL_MS);
        CallAfter([this] { RunArchiveCleanup(); });

        _settingsView->Reload();
        ShowCurrentView();
        if (_navBar->IsShown())
            _navBar->FocusExpansion(_expansion);
    }

    MainFrame::~MainFrame()
    {
        _cleanupTimer.Stop();
        if (_trayIcon)
        {
            _trayIcon->RemoveIcon();
            _trayIcon->Destroy();
        }
    }

    void MainFrame::ShowFromTray()
    {
        if (!IsShown())
            Show();
        if (IsIconized())
            Iconize(false);
        Raise();
    }

    void MainFrame::ShowCurrentView()
    {
        bool hasRoot = !_settings.WowRoot.empty();
        _navBar->Show(hasRoot);
        _navLine->Show(hasRoot);
        _navBar->SetSelection(_expansion, _subView);

        if (!hasRoot)
        {
            _rootPrompt->SetPrompt("Select your World of Warcraft folder",
                wxString(Core::AppInfo::NAME) + " needs your World of Warcraft folder to find your combat logs. "
                "It's the folder that has _classic_era_, _anniversary_ or _classic_ inside.",
                wxEmptyString);
            _book->SetSelection(PAGE_ROOT_PROMPT);
        }
        else if (_subView == SubView::Settings)
        {
            _book->SetSelection(PAGE_SETTINGS);
        }
        else if (const wxString& folder = _settings.ExpansionFolder(_expansion); folder.empty())
        {
            _expansionPrompt->SetPrompt("Select your " + Core::ExpansionName(_expansion) + " folder",
                "Choose the " + Core::ExpansionName(_expansion) + " client folder inside your World of Warcraft folder."
                + ClientFolderHint(_expansion) + " You can change it later in Settings.",
                _settings.WowRoot);
            _book->SetSelection(PAGE_EXPANSION_PROMPT);
        }
        else
        {
            _logsView->ShowExpansion(_expansion, folder);
            _book->SetSelection(PAGE_LOGS);
        }

        _book->GetParent()->Layout();
    }

    void MainFrame::OnExpansionSelected(Core::Expansion expansion)
    {
        _expansion = expansion;
        _settings.LastExpansion = expansion;
        _settings.Save();
        ShowCurrentView();
    }

    void MainFrame::OnSubViewSelected(SubView subView)
    {
        _subView = subView;
        ShowCurrentView();
    }

    void MainFrame::OnRootFolderChosen(const wxString& folder)
    {
        if (!Core::WowFolders::LooksLikeWowRoot(folder)
            && wxMessageBox("This doesn't look like a World of Warcraft folder: there's no _classic_era_, _anniversary_, _classic_ or _classic_beta_ inside it.\n\nUse it anyway?",
                Core::AppInfo::NAME, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) != wxYES)
            return;

        Core::WowFolders::ChangeRoot(_settings, folder);
        _settings.Save();
        _settingsView->Reload();
        ShowCurrentView();
    }

    void MainFrame::OnExpansionFolderChosen(const wxString& folder)
    {
        _settings.ExpansionFolder(_expansion) = folder;
        _settings.Save();
        _settingsView->Reload();
        ShowCurrentView();
    }

    void MainFrame::OnClose(wxCloseEvent& event)
    {
        if (event.CanVeto() && _settings.CloseToTray && _trayIcon)
        {
            Hide();
            event.Veto();
            return;
        }

        Destroy();
    }

    void MainFrame::RunArchiveCleanup()
    {
        if (!_settings.AutoArchive || _settings.AutoDeleteArchiveDays <= 0)
            return;

        for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
        {
            const wxString& folder = _settings.ExpansionFolder(expansion);
            if (!folder.empty())
                Core::LogArchive::DeleteArchivedOlderThan(expansion, folder, _settings.AutoDeleteArchiveDays);
        }
    }
}
