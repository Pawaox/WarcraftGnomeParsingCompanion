#pragma once

#include "Core/CombatLogs.h"
#include "Core/Expansion.h"
#include "Core/Settings.h"
#include "Ui/Controls/Button.h"
#include "Ui/Controls/ToggleSwitch.h"

#include <wx/fswatcher.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>

#include <functional>
#include <memory>
#include <vector>

namespace Ui
{
    class LogsView : public wxPanel
    {
    public:
        LogsView(wxWindow* parent, Core::Settings& settings, std::function<void(const wxString&)> onArchive, std::function<void(const wxString&)> onUpload);

        void ShowExpansion(Core::Expansion expansion, const wxString& clientFolder);
        void Reload();

    private:
        struct Row
        {
            wxString Path;
            wxStaticText* Date = nullptr;
            wxStaticText* Length = nullptr;
            wxStaticText* Size = nullptr;
            wxStaticText* Status = nullptr;
            Controls::Button* Upload = nullptr;
            Controls::Button* Archive = nullptr;
        };

        wxStaticText* NewColumn(wxSizer* sizer, const wxString& text, int width);
        void AddHeaderRow();
        void AddRow(const Core::CombatLogs::CombatLogFile& file);
        void UpdateRow(Row& row, const Core::CombatLogs::CombatLogFile& file);
        void RebuildRows(const std::vector<Core::CombatLogs::CombatLogFile>& files);
        void WatchLogsFolder();
        void OnFileEvent(wxFileSystemWatcherEvent& event);
        void ReloadIfVisible();
        void OnAutoRefreshClicked();
        void UpdateAutoRefreshToggle();
        void ScheduleLockCheck();

        Core::Settings& _settings;
        std::function<void(const wxString&)> _onArchive;
        std::function<void(const wxString&)> _onUpload;
        wxString _clientFolder;
        std::vector<Row> _rows;
        std::unique_ptr<wxFileSystemWatcher> _watcher;
        wxTimer _reloadTimer;
        wxTimer _lockCheckTimer;
        wxString _lastListState;
        wxDateTime _lastUpdate;
        bool _anyLocked = false;
        bool _autoRefreshIdle = false;

        wxStaticText* _title = nullptr;
        wxStaticText* _folder = nullptr;
        Controls::ToggleSwitch* _autoRefresh = nullptr;
        wxScrolledWindow* _list = nullptr;
        wxBoxSizer* _listSizer = nullptr;
    };
}
