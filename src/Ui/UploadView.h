#pragma once

#include "Core/Api.h"
#include "Core/Expansion.h"
#include "Core/LogScanner.h"
#include "Ui/Controls/Button.h"
#include "Ui/Controls/Dropdown.h"
#include "Ui/Controls/SelectRow.h"
#include "Ui/Controls/ToggleSwitch.h"

#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace Ui
{
    class UploadView : public wxPanel
    {
    public:
        UploadView(wxWindow* parent, std::function<void()> onClosed, std::function<void(const std::vector<wxString>&)> onUploaded);

        void Start(Core::Expansion expansion, const std::vector<wxString>& paths, const std::vector<Core::Api::Guild>& guilds, const std::vector<Core::Api::ExpansionInfo>& expansions, const wxString& token);
        bool IsBusy() const { return _busy; }

    private:
        wxTextCtrl* NewInput();
        void OnScanFinished(const Core::LogScanner::ScanResult& result);
        void OnScanProgress(int percent);
        void OnSelectionChanged();
        void OnSelectAll();
        void OnUpload();
        void OnCancel();
        void OnUploadProgress(const wxString& stage, int percent);
        void OnUploadFinished(bool ok, const wxString& message);
        void SetStatus(const wxString& status);
        void ClearRows();

        std::function<void()> _onClosed;
        std::function<void(const std::vector<wxString>&)> _onUploaded;
        std::shared_ptr<std::atomic<bool>> _cancelled;
        Core::LogScanner::ScanResult _scan;
        std::vector<Core::Api::Guild> _guilds;
        long long _expansionID = 0;
        std::vector<Controls::SelectRow*> _rows;
        wxString _token;
        bool _busy = false;

        wxStaticText* _status = nullptr;
        wxTextCtrl* _name = nullptr;
        wxTextCtrl* _warcraftLogs = nullptr;
        Controls::Dropdown* _guild = nullptr;
        Controls::ToggleSwitch* _public = nullptr;
        Controls::Button* _selectAll = nullptr;
        Controls::Button* _upload = nullptr;
        Controls::Button* _cancel = nullptr;
        wxScrolledWindow* _list = nullptr;
        wxBoxSizer* _listSizer = nullptr;
    };
}
