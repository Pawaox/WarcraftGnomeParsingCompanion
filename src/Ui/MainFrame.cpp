#include "Ui/MainFrame.h"

#include "Core/AppInfo.h"
#include "Core/CombatLogs.h"
#include "Core/LogArchive.h"
#include "Core/TokenStore.h"
#include "Core/WowFolders.h"
#include "Ui/Controls/Separator.h"
#include "Ui/Fonts.h"
#include "Ui/Images.h"
#include "Ui/LogPickerDialog.h"
#include "Ui/Palette.h"

#include <wx/filefn.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/taskbar.h>

#include <algorithm>
#include <iterator>
#include <thread>

namespace Ui
{
    namespace
    {
        constexpr int CLEANUP_INTERVAL_MS = 60 * 60 * 1000;
        constexpr int ARCHIVE_QUEUE_INTERVAL_MS = 30 * 1000;

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

        const Palette& palette = Palette::Current();
        auto* root = new wxPanel(this);
        root->SetBackgroundColour(palette.Background);
        root->SetForegroundColour(palette.Text);
        root->SetFont(Fonts::Regular());

        _navBar = new NavBar(root,
            [this](Core::Expansion expansion) { OnExpansionSelected(expansion); },
            [this](SubView subView) { OnSubViewSelected(subView); });
        _navLine = new Controls::Separator(root, wxHORIZONTAL, palette.NavBorder);

        _book = new wxSimplebook(root);
        _book->SetBackgroundColour(palette.Background);
        _rootPrompt = new FolderPromptView(_book, [this](const wxString& folder) { OnRootFolderChosen(folder); });
        _expansionPrompt = new FolderPromptView(_book, [this](const wxString& folder) { OnExpansionFolderChosen(folder); });
        _logsView = new LogsView(_book, _settings, [this](const wxString& path) { ArchiveLog(path); }, [this](const wxString& path) { BeginUpload(path); });
        _settingsView = new SettingsView(_book, _settings, [this] { OnLogout(); },
            [this] { return _expansion; },
            [this] { ShowCurrentView(); });
        _book->AddPage(_rootPrompt, wxEmptyString);
        _book->AddPage(_expansionPrompt, wxEmptyString);
        _book->AddPage(_logsView, wxEmptyString);
        _book->AddPage(_settingsView, wxEmptyString);
        _loginView = new LoginView(_book, [this](const wxString& token) { OnLoggedIn(token); });
        _uploadView = new UploadView(_book,
            [this] { _showUpload = false; ShowCurrentView(); },
            [this](const std::vector<wxString>& paths) { OnUploaded(paths); });
        _book->AddPage(_loginView, wxEmptyString);
        _book->AddPage(_uploadView, wxEmptyString);

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
        _archiveQueueTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { OnArchiveQueueTimer(); });
        UpdateArchiveQueueTimer();

        if (!_settings.WebsiteUrl.empty())
            Core::Api::SetBaseUrl(_settings.WebsiteUrl);

        _token = Core::TokenStore::Load();
        if (!_token.empty())
            CallAfter([this] { LoadProfile(); });

        _settingsView->Reload();
        ShowCurrentView();
    }

    MainFrame::~MainFrame()
    {
        _cleanupTimer.Stop();
        _archiveQueueTimer.Stop();
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
        ShowCurrentView();
    }

    void MainFrame::ShowCurrentView()
    {
        bool loggedIn = !_token.empty();
        bool hasRoot = loggedIn && !_settings.WowRoot.empty();
        _navBar->Show(hasRoot);
        _navLine->Show(hasRoot);
        _navBar->SetSelection(_expansion, _subView);
        _settingsView->SetActionsExpansion(_expansion);

        if (!loggedIn)
        {
            _book->SetSelection(PAGE_LOGIN);
        }
        else if (_showUpload)
        {
            _book->SetSelection(PAGE_UPLOAD);
        }
        else if (!hasRoot)
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
        if (_uploadView->IsBusy())
            return;

        _showUpload = false;
        _expansion = expansion;
        _settings.LastExpansion = expansion;
        _settings.Save();
        ShowCurrentView();
    }

    void MainFrame::OnSubViewSelected(SubView subView)
    {
        if (_uploadView->IsBusy())
            return;

        _showUpload = false;
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

    void MainFrame::OnArchiveQueueTimer()
    {
        bool changed = ProcessArchiveQueue();
        UpdateArchiveQueueTimer();
        if (changed && IsShown() && !IsIconized() && _book->GetSelection() == PAGE_LOGS)
            _logsView->Reload();
    }

    void MainFrame::UpdateArchiveQueueTimer()
    {
        if (_settings.ArchiveQueue.empty())
            _archiveQueueTimer.Stop();
        else if (!_archiveQueueTimer.IsRunning())
            _archiveQueueTimer.Start(ARCHIVE_QUEUE_INTERVAL_MS);
    }

    void MainFrame::ArchiveLog(const wxString& path)
    {
        Core::LogArchive::ArchiveOutcome outcome = Core::LogArchive::ArchiveFile(_expansion, _settings.ExpansionFolder(_expansion), path);
        if (outcome == Core::LogArchive::ArchiveOutcome::InUse)
        {
            _settings.ArchiveQueue.push_back({ _expansion, path });
            _settings.Save();
            UpdateArchiveQueueTimer();
        }
        else if (outcome == Core::LogArchive::ArchiveOutcome::Failed)
        {
            wxMessageBox("Couldn't move this log to the archive:\n\n" + path, Core::AppInfo::NAME, wxOK | wxICON_ERROR, this);
        }

        _logsView->Reload();
    }

    bool MainFrame::ProcessArchiveQueue()
    {
        size_t before = _settings.ArchiveQueue.size();
        std::erase_if(_settings.ArchiveQueue, [this](const Core::QueuedArchive& queued)
        {
            if (!wxFileExists(queued.Path))
                return true;

            const wxString& folder = _settings.ExpansionFolder(queued.LogExpansion);
            return Core::LogArchive::ArchiveFile(queued.LogExpansion, folder, queued.Path) != Core::LogArchive::ArchiveOutcome::InUse;
        });

        if (_settings.ArchiveQueue.size() == before)
            return false;

        _settings.Save();
        return true;
    }

    void MainFrame::OnLoggedIn(const wxString& token)
    {
        _token = token;
        Core::TokenStore::Save(token);
        ShowCurrentView();
        LoadProfile();
    }

    void MainFrame::OnLogout()
    {
        wxString token = _token;
        std::thread([token] { Core::Api::Post("/api/companion/logout", nlohmann::json::object(), token); }).detach();

        _token.clear();
        _guilds.clear();
        _expansions.clear();
        _showUpload = false;
        Core::TokenStore::Clear();
        ShowCurrentView();
    }

    void MainFrame::LoadProfile()
    {
        std::thread([this, token = _token]
        {
            Core::Api::Response response = Core::Api::Post("/api/companion/profile", nlohmann::json::object(), token);
            CallAfter([this, response]
            {
                if (response.Ok)
                {
                    _guilds = Core::Api::ParseGuilds(response.Result);
                    _expansions = Core::Api::ParseExpansions(response.Result);
                    return;
                }

                _token.clear();
                _loginView->ShowMessage("Please log in again. " + response.Error);
                ShowCurrentView();
            });
        }).detach();
    }

    void MainFrame::BeginUpload(const wxString& path)
    {
        const wxString& folder = _settings.ExpansionFolder(_expansion);
        std::vector<Core::CombatLogs::CombatLogFile> files = Core::CombatLogs::Find(Core::WowFolders::LogsDirectory(folder));

        auto selected = std::ranges::find_if(files, [&path](const auto& file) { return file.Path == path; });
        if (selected == files.end())
            return;

        std::vector<Core::CombatLogs::CombatLogFile> others;
        std::ranges::copy_if(files, std::back_inserter(others), [&path](const auto& file)
        {
            return file.Path != path && !file.InUse && file.Size > 0;
        });

        std::vector<wxString> paths = { path };
        if (!others.empty())
        {
            LogPickerDialog dialog(this, *selected, others);
            if (dialog.ShowModal() != wxID_OK)
                return;
            paths = dialog.SelectedPaths();
        }

        auto position = [&files](const wxString& value)
        {
            return std::ranges::find_if(files, [&value](const auto& file) { return file.Path == value; }) - files.begin();
        };
        std::ranges::sort(paths, [&position](const wxString& left, const wxString& right) { return position(left) > position(right); });

        _showUpload = true;
        _uploadView->Start(_expansion, paths, _guilds, _expansions, _token);
        ShowCurrentView();
    }

    void MainFrame::OnUploaded(const std::vector<wxString>& paths)
    {
        if (!_settings.AutoArchive)
            return;

        for (const wxString& path : paths)
            ArchiveLog(path);
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
