#include "Ui/LogsView.h"

#include "Core/WowFolders.h"
#include "Ui/Fonts.h"
#include "Ui/Palette.h"

#include <wx/filename.h>
#include <wx/log.h>

#include <algorithm>

namespace Ui
{
    namespace
    {
        constexpr int MARGIN = 16;
        constexpr int ROW_SPACING = 12;
        constexpr int LENGTH_WIDTH = 70;
        constexpr int SIZE_WIDTH = 80;
        constexpr int STATUS_WIDTH = 110;
        constexpr int BUTTON_WIDTH = 74;
        constexpr int BUTTON_SPACING = 6;
        constexpr double NAME_SIZE = 9.5;
        constexpr int LOCKED_AFTER_MINUTES = 5;
        constexpr int RELOAD_DELAY_MS = 2 * 1000;
        constexpr int LOCK_CHECK_INTERVAL_MS = 60 * 1000;
        constexpr int AUTO_REFRESH_IDLE_HOURS = 1;

        const wxString DATE_FORMAT = "%d %b %Y  %H:%M";

        const wxDateTime& DisplayTime(const Core::CombatLogs::CombatLogFile& file)
        {
            return file.Started.IsValid() ? file.Started : file.Modified;
        }

        wxString LengthText(const Core::CombatLogs::CombatLogFile& file)
        {
            if (!file.Started.IsValid() || !file.Modified.IsValid() || file.Modified < file.Started)
                return "-";

            int minutes = (file.Modified - file.Started).GetMinutes();
            return minutes >= 60 ? wxString::Format("%dh %02dm", minutes / 60, minutes % 60) : wxString::Format("%dm", minutes);
        }

        bool WasRecentlyModified(const Core::CombatLogs::CombatLogFile& file)
        {
            return file.Modified.IsValid() && file.Modified > wxDateTime::Now() - wxTimeSpan::Minutes(LOCKED_AFTER_MINUTES);
        }

        wxString StatusText(const Core::CombatLogs::CombatLogFile& file, bool queued)
        {
            if (queued)
                return "Archive queued";
            if (!file.InUse)
                return "Ready";
            return WasRecentlyModified(file) ? "Logging" : "Locked";
        }

        wxString ListState(const std::vector<Core::CombatLogs::CombatLogFile>& files)
        {
            wxString state;
            for (const Core::CombatLogs::CombatLogFile& file : files)
                state << file.Path << '|' << file.Size.ToString() << '|' << file.Modified.GetTicks() << '|' << file.InUse << ';';
            return state;
        }

        wxString StatusTooltip()
        {
            return wxString::Format("Logging = File is locked and was recently modified\nLocked = File is locked but hasn't been modified for %d minutes",
                LOCKED_AFTER_MINUTES);
        }
    }

    LogsView::LogsView(wxWindow* parent, Core::Settings& settings, std::function<void(const wxString&)> onArchive, std::function<void(const wxString&)> onUpload)
        : wxPanel(parent),
          _settings(settings),
          _onArchive(std::move(onArchive)),
          _onUpload(std::move(onUpload)),
          _lastUpdate(wxDateTime::Now())
    {
        const Palette& palette = Palette::Current();
        SetBackgroundColour(palette.Background);

        _title = new wxStaticText(this, wxID_ANY, wxEmptyString);
        _title->SetFont(Fonts::Bold(13.0));
        _title->SetForegroundColour(palette.Title);

        _folder = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
        _folder->SetForegroundColour(palette.TextInactive);

        auto* refresh = new Controls::Button(this, "Refresh", [this] { Reload(); });

        auto* autoLabel = new wxStaticText(this, wxID_ANY, "Auto");
        autoLabel->SetForegroundColour(palette.TextInactive);
        autoLabel->SetToolTip("Auto refresh list every minute.\nTurns off after 1 hour of no update");
        _autoRefresh = new Controls::ToggleSwitch(this, [this] { OnAutoRefreshClicked(); });
        _autoRefresh->SetToolTip("Auto refresh list every minute.\nTurns off after 1 hour of no update");
        UpdateAutoRefreshToggle();

        _list = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
        _list->SetBackgroundColour(palette.Background);
        _list->SetScrollRate(0, FromDIP(12));
        _listSizer = new wxBoxSizer(wxVERTICAL);
        _list->SetSizer(_listSizer);

        auto* header = new wxBoxSizer(wxHORIZONTAL);
        header->Add(_title, 1, wxALIGN_CENTER_VERTICAL);
        header->Add(autoLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(BUTTON_SPACING));
        header->Add(_autoRefresh, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(ROW_SPACING));
        header->Add(refresh, 0, wxALIGN_CENTER_VERTICAL);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(header, 0, wxEXPAND | wxALL, FromDIP(MARGIN));
        sizer->Add(_folder, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(MARGIN));
        sizer->Add(_list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(MARGIN));
        SetSizer(sizer);

        Bind(wxEVT_FSWATCHER, &LogsView::OnFileEvent, this);
        _reloadTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { ReloadIfVisible(); });
        _lockCheckTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { ReloadIfVisible(); });
    }

    void LogsView::ShowExpansion(Core::Expansion expansion, const wxString& clientFolder)
    {
        _title->SetLabelText(Core::ExpansionName(expansion) + " logs");
        _folder->SetLabelText("Logs folder: " + Core::WowFolders::LogsDirectory(clientFolder));
        bool folderChanged = _clientFolder != clientFolder;
        _clientFolder = clientFolder;
        Reload();
        Layout();

        if (folderChanged)
            CallAfter([this] { WatchLogsFolder(); });
    }

    void LogsView::WatchLogsFolder()
    {
        wxLogNull noLog;
        if (!_watcher)
        {
            _watcher = std::make_unique<wxFileSystemWatcher>();
            _watcher->SetOwner(this);
        }

        _watcher->RemoveAll();
        wxString logsDirectory = Core::WowFolders::LogsDirectory(_clientFolder);
        if (wxDirExists(logsDirectory))
            _watcher->Add(wxFileName::DirName(logsDirectory), wxFSW_EVENT_CREATE | wxFSW_EVENT_DELETE | wxFSW_EVENT_RENAME | wxFSW_EVENT_MODIFY);
    }

    void LogsView::OnFileEvent(wxFileSystemWatcherEvent& event)
    {
        bool isLog = event.GetPath().GetExt().IsSameAs("txt", false) || event.GetNewPath().GetExt().IsSameAs("txt", false);
        if (isLog && !_reloadTimer.IsRunning())
            _reloadTimer.StartOnce(RELOAD_DELAY_MS);
    }

    void LogsView::ReloadIfVisible()
    {
        if (IsShownOnScreen())
            Reload();
    }

    void LogsView::Reload()
    {
        if (_clientFolder.empty())
            return;

        std::vector<Core::CombatLogs::CombatLogFile> files = Core::CombatLogs::Find(Core::WowFolders::LogsDirectory(_clientFolder));

        bool sameFiles = !files.empty() && std::ranges::equal(files, _rows, [](const auto& file, const Row& row) { return file.Path == row.Path; });
        if (sameFiles)
        {
            for (size_t index = 0; index < files.size(); ++index)
                UpdateRow(_rows[index], files[index]);
        }
        else
        {
            RebuildRows(files);
        }

        wxString listState = ListState(files);
        if (listState != _lastListState)
        {
            _lastListState = listState;
            _lastUpdate = wxDateTime::Now();
            _autoRefreshIdle = false;
        }

        _anyLocked = std::ranges::any_of(files, [](const auto& file) { return file.InUse; });
        ScheduleLockCheck();
    }

    void LogsView::ScheduleLockCheck()
    {
        if (_settings.AutoRefresh && !_autoRefreshIdle && wxDateTime::Now() - _lastUpdate >= wxTimeSpan::Hours(AUTO_REFRESH_IDLE_HOURS))
            _autoRefreshIdle = true;

        UpdateAutoRefreshToggle();

        if (!_settings.AutoRefresh || _autoRefreshIdle || !_anyLocked)
            _lockCheckTimer.Stop();
        else if (!_lockCheckTimer.IsRunning())
            _lockCheckTimer.StartOnce(LOCK_CHECK_INTERVAL_MS);
    }

    void LogsView::OnAutoRefreshClicked()
    {
        if (_autoRefreshIdle)
        {
            _autoRefreshIdle = false;
            _lastUpdate = wxDateTime::Now();
        }
        else
        {
            _settings.AutoRefresh = !_settings.AutoRefresh;
            _lastUpdate = wxDateTime::Now();
            _settings.Save();
        }

        ScheduleLockCheck();
    }

    void LogsView::UpdateAutoRefreshToggle()
    {
        _autoRefresh->SetValue(_settings.AutoRefresh && !_autoRefreshIdle);
    }

    wxStaticText* LogsView::NewColumn(wxSizer* sizer, const wxString& text, int width)
    {
        auto* label = new wxStaticText(_list, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
        label->SetLabelText(text);
        label->SetForegroundColour(Palette::Current().Text);
        label->SetMinSize(wxSize(FromDIP(width), -1));
        sizer->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(ROW_SPACING));
        return label;
    }

    void LogsView::AddHeaderRow()
    {
        const Palette& palette = Palette::Current();
        auto* header = new wxBoxSizer(wxHORIZONTAL);

        auto* date = new wxStaticText(_list, wxID_ANY, "Started");
        date->SetForegroundColour(palette.Accent);
        header->Add(date, 1, wxALIGN_CENTER_VERTICAL);

        for (const auto& [text, width] : { std::pair<wxString, int>{ "Length", LENGTH_WIDTH }, { "Size", SIZE_WIDTH }, { "Status", STATUS_WIDTH } })
            NewColumn(header, text, width)->SetForegroundColour(palette.Accent);

        header->AddSpacer(FromDIP(ROW_SPACING + BUTTON_WIDTH * 2 + BUTTON_SPACING));
        _listSizer->Add(header, 0, wxEXPAND | wxBOTTOM, FromDIP(8));
    }

    void LogsView::AddRow(const Core::CombatLogs::CombatLogFile& file)
    {
        const Palette& palette = Palette::Current();
        Row row;
        row.Path = file.Path;

        auto* line = new wxBoxSizer(wxHORIZONTAL);

        auto* nameColumn = new wxBoxSizer(wxVERTICAL);
        row.Date = new wxStaticText(_list, wxID_ANY, wxEmptyString);
        row.Date->SetFont(Fonts::SemiBold());
        row.Date->SetForegroundColour(palette.Text);
        nameColumn->Add(row.Date, 0, wxEXPAND);

        auto* name = new wxStaticText(_list, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);
        name->SetLabelText(file.Name);
        name->SetFont(Fonts::Regular(NAME_SIZE));
        name->SetForegroundColour(palette.TextInactive);
        name->SetMinSize(wxSize(1, -1));
        nameColumn->Add(name, 0, wxEXPAND);
        line->Add(nameColumn, 1, wxALIGN_CENTER_VERTICAL);

        row.Length = NewColumn(line, wxEmptyString, LENGTH_WIDTH);
        row.Size = NewColumn(line, wxEmptyString, SIZE_WIDTH);
        row.Status = NewColumn(line, wxEmptyString, STATUS_WIDTH);

        row.Upload = new Controls::Button(_list, "Upload", [this, path = file.Path] { _onUpload(path); }, Controls::Button::Style::Primary);
        row.Upload->SetMinSize(wxSize(FromDIP(BUTTON_WIDTH), -1));
        line->Add(row.Upload, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(ROW_SPACING));

        row.Archive = new Controls::Button(_list, "Archive", [this, path = file.Path] { _onArchive(path); });
        row.Archive->SetMinSize(wxSize(FromDIP(BUTTON_WIDTH), -1));
        line->Add(row.Archive, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(BUTTON_SPACING));

        _listSizer->Add(line, 0, wxEXPAND | wxBOTTOM, FromDIP(ROW_SPACING));

        UpdateRow(row, file);
        _rows.push_back(row);
    }

    void LogsView::UpdateRow(Row& row, const Core::CombatLogs::CombatLogFile& file)
    {
        const Palette& palette = Palette::Current();
        bool queued = _settings.IsArchiveQueued(file.Path);

        const wxDateTime& time = DisplayTime(file);
        wxString date = time.IsValid() ? time.Format(DATE_FORMAT) : wxString("Unknown date");
        if (row.Date->GetLabelText() != date)
            row.Date->SetLabelText(date);

        wxString length = LengthText(file);
        if (row.Length->GetLabelText() != length)
            row.Length->SetLabelText(length);

        wxString size = wxFileName::GetHumanReadableSize(file.Size, "0 B");
        if (row.Size->GetLabelText() != size)
            row.Size->SetLabelText(size);

        wxString status = StatusText(file, queued);
        if (row.Status->GetLabelText() != status)
        {
            row.Status->SetLabelText(status);
            row.Status->SetForegroundColour(queued ? palette.TextInactive : file.InUse ? palette.Accent : palette.Text);
            row.Status->SetToolTip(file.InUse && !queued ? StatusTooltip() : wxString());
            row.Status->Refresh();
        }

        row.Upload->Enable(!file.InUse && !queued && file.Size > 0);
        row.Upload->SetToolTip(file.InUse ? wxString("World of Warcraft still has this log open.") : wxString());
        row.Archive->Enable(!queued);
    }

    void LogsView::RebuildRows(const std::vector<Core::CombatLogs::CombatLogFile>& files)
    {
        Freeze();
        _rows.clear();
        _listSizer->Clear(true);

        if (files.empty())
        {
            auto* empty = new wxStaticText(_list, wxID_ANY, "No combat logs in this folder.");
            empty->SetForegroundColour(Palette::Current().TextInactive);
            _listSizer->Add(empty);
        }
        else
        {
            AddHeaderRow();
            for (const Core::CombatLogs::CombatLogFile& file : files)
                AddRow(file);
        }

        _list->FitInside();
        _list->Layout();
        Thaw();
    }

}
