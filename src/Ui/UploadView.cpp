#include "Ui/UploadView.h"

#include "Core/LogUpload.h"
#include "Ui/Fonts.h"
#include "Ui/Palette.h"

#include <wx/filename.h>

#include <algorithm>
#include <thread>

namespace Ui
{
    namespace
    {
        constexpr int MARGIN = 16;
        constexpr int ROW_GAP = 8;
        constexpr int FIELD_GAP = 12;
        constexpr int INPUT_WIDTH = 220;

        const wxString NO_GUILD = "No guild";
        const wxString TIME_FORMAT = "%d %b  %H:%M";
        const wxString END_FORMAT = "%H:%M";

        wxString GroupDetail(const Core::LogScanner::LogGroup& group)
        {
            wxString detail;
            if (group.Started.IsValid() && group.Ended.IsValid())
                detail = group.Started.Format(TIME_FORMAT) + " - " + group.Ended.Format(END_FORMAT);

            if (group.Kills > 0)
                detail += wxString::Format("   %d killed", group.Kills);
            if (group.Wipes > 0)
                detail += wxString::Format("   %d wiped", group.Wipes);

            return detail + "   " + wxFileName::GetHumanReadableSize(group.Bytes, "0 B");
        }

        wxString GroupTitle(const Core::LogScanner::LogGroup& group)
        {
            return group.Character.empty() ? group.Zone : group.Zone + "  -  " + group.Character;
        }
    }

    UploadView::UploadView(wxWindow* parent, std::function<void()> onClosed, std::function<void(const std::vector<wxString>&)> onUploaded)
        : wxPanel(parent),
          _onClosed(std::move(onClosed)),
          _onUploaded(std::move(onUploaded))
    {
        const Palette& palette = Palette::Current();
        SetBackgroundColour(palette.Background);

        auto* title = new wxStaticText(this, wxID_ANY, "Upload");
        title->SetFont(Fonts::Bold(13.0));
        title->SetForegroundColour(palette.Title);

        _status = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
        _status->SetForegroundColour(palette.TextInactive);

        _name = NewInput();
        _name->SetHint("Name of the log");
        _warcraftLogs = NewInput();
        _warcraftLogs->SetHint("Warcraft Logs link (optional)");
        _guild = new Controls::Dropdown(this, { NO_GUILD }, [](int) {});
        _public = new Controls::ToggleSwitch(this, [this] { _public->SetValue(!_public->GetValue()); });
        _public->SetValue(true);

        auto* publicLabel = new wxStaticText(this, wxID_ANY, "Public");
        publicLabel->SetForegroundColour(palette.TextInactive);

        auto* fields = new wxBoxSizer(wxHORIZONTAL);
        fields->Add(_name, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(FIELD_GAP));
        fields->Add(_warcraftLogs, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(FIELD_GAP));
        fields->Add(_guild, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(FIELD_GAP));
        fields->Add(publicLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
        fields->Add(_public, 0, wxALIGN_CENTER_VERTICAL);

        _selectAll = new Controls::Button(this, "Select All", [this] { OnSelectAll(); });
        _upload = new Controls::Button(this, "Upload", [this] { OnUpload(); }, Controls::Button::Style::Primary);
        _cancel = new Controls::Button(this, "Back", [this] { OnCancel(); });

        auto* actions = new wxBoxSizer(wxHORIZONTAL);
        actions->Add(_selectAll, 0, wxALIGN_CENTER_VERTICAL);
        actions->AddStretchSpacer();
        actions->Add(_cancel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(8));
        actions->Add(_upload, 0, wxALIGN_CENTER_VERTICAL);

        _list = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
        _list->SetBackgroundColour(palette.Background);
        _list->SetScrollRate(0, FromDIP(12));
        _listSizer = new wxBoxSizer(wxVERTICAL);
        _list->SetSizer(_listSizer);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(title, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));
        sizer->Add(_status, 0, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(MARGIN));
        sizer->Add(fields, 0, wxEXPAND | wxALL, FromDIP(MARGIN));
        sizer->Add(_list, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(MARGIN));
        sizer->Add(actions, 0, wxEXPAND | wxALL, FromDIP(MARGIN));
        SetSizer(sizer);
    }

    wxTextCtrl* UploadView::NewInput()
    {
        const Palette& palette = Palette::Current();
        auto* input = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
        input->SetBackgroundColour(palette.InputBackground);
        input->SetForegroundColour(palette.InputText);
        input->SetMinSize(wxSize(FromDIP(INPUT_WIDTH / 2), -1));
        return input;
    }

    void UploadView::Start(Core::Expansion expansion, const std::vector<wxString>& paths, const std::vector<Core::Api::Guild>& guilds, const std::vector<Core::Api::ExpansionInfo>& expansions, const wxString& token)
    {
        _token = token;
        _busy = true;
        _guilds.clear();
        _expansionID = 0;

        wxArrayString guildLabels;
        guildLabels.Add(NO_GUILD);
        wxString expansionKey = Core::ExpansionKey(expansion);
        for (const Core::Api::ExpansionInfo& info : expansions)
        {
            if (info.Code.IsSameAs(expansionKey, false))
                _expansionID = info.ID;
        }

        for (const Core::Api::Guild& guild : guilds)
        {
            if (guild.ExpansionCode.IsSameAs(expansionKey, false))
            {
                _guilds.push_back(guild);
                guildLabels.Add(guild.Name);
            }
        }

        _guild->SetItems(guildLabels);
        _guild->SetSelection(_guilds.empty() ? 0 : 1);
        _name->Clear();
        _warcraftLogs->Clear();
        _public->SetValue(true);
        ClearRows();
        _selectAll->Enable(false);
        _upload->Enable(false);
        _cancel->SetLabelText("Cancel");
        SetStatus("Reading files...");

        auto cancelled = std::make_shared<std::atomic<bool>>(false);
        _cancelled = cancelled;

        std::thread([this, paths, cancelled]
        {
            int lastPercent = -1;
            Core::LogScanner::ScanResult result = Core::LogScanner::Scan(paths, [this, &lastPercent, cancelled](int percent)
            {
                if (percent != lastPercent)
                {
                    lastPercent = percent;
                    CallAfter([this, percent] { OnScanProgress(percent); });
                }
            }, *cancelled);

            if (!*cancelled)
                CallAfter([this, result] { OnScanFinished(result); });
        }).detach();
    }

    void UploadView::OnScanProgress(int percent)
    {
        if (_busy)
            SetStatus(wxString::Format("Reading files... %d%%", percent));
    }

    void UploadView::OnScanFinished(const Core::LogScanner::ScanResult& result)
    {
        _busy = false;
        _scan = result;
        _cancel->SetLabelText("Back");

        if (!result.Error.empty())
        {
            SetStatus(result.Error);
            return;
        }

        _list->Freeze();
        for (const Core::LogScanner::LogGroup& group : result.Groups)
        {
            auto* row = new Controls::SelectRow(_list, GroupTitle(group), GroupDetail(group), [this] { OnSelectionChanged(); });
            _listSizer->Add(row, 0, wxEXPAND | wxBOTTOM, FromDIP(ROW_GAP));
            _rows.push_back(row);
        }
        _list->FitInside();
        _list->Layout();
        _list->Thaw();

        _selectAll->Enable(!_rows.empty());
        SetStatus(_rows.empty() ? wxString("Nothing to upload in this log.") : wxString("Select what you want to upload."));
    }

    void UploadView::OnSelectionChanged()
    {
        bool any = std::ranges::any_of(_rows, [](const Controls::SelectRow* row) { return row->IsSelected(); });
        _upload->Enable(any && !_busy);
    }

    void UploadView::OnSelectAll()
    {
        bool all = std::ranges::all_of(_rows, [](const Controls::SelectRow* row) { return row->IsSelected(); });
        for (Controls::SelectRow* row : _rows)
            row->SetSelected(!all);
        OnSelectionChanged();
    }

    void UploadView::OnUpload()
    {
        Core::LogUpload::UploadRequest request;
        request.Scan = _scan;
        request.Token = _token;
        request.Name = _name->GetValue().Trim().Trim(false);
        request.WarcraftLogsID = _warcraftLogs->GetValue().Trim().Trim(false);
        request.IsPublic = _public->GetValue();

        request.ExpansionID = _expansionID;

        int guildIndex = _guild->GetSelection() - 1;
        if (guildIndex >= 0 && guildIndex < static_cast<int>(_guilds.size()))
        {
            request.GuildID = _guilds[guildIndex].ID;
            request.ExpansionID = _guilds[guildIndex].ExpansionID;
        }

        for (size_t index = 0; index < _rows.size(); ++index)
        {
            if (_rows[index]->IsSelected())
                request.SelectedGroups.push_back(index);
        }

        _busy = true;
        _upload->Enable(false);
        _selectAll->Enable(false);
        _cancel->SetLabelText("Cancel");

        auto cancelled = std::make_shared<std::atomic<bool>>(false);
        _cancelled = cancelled;

        std::thread([this, request = std::move(request), cancelled]
        {
            int lastPercent = -1;
            Core::LogUpload::UploadResult result = Core::LogUpload::Run(request, [this, &lastPercent, cancelled](const wxString& stage, int percent)
            {
                if (percent != lastPercent)
                {
                    lastPercent = percent;
                    CallAfter([this, stage, percent] { OnUploadProgress(stage, percent); });
                }
            }, *cancelled);

            if (!*cancelled)
                CallAfter([this, result] { OnUploadFinished(result.Ok, result.Message); });
        }).detach();
    }

    void UploadView::OnUploadProgress(const wxString& stage, int percent)
    {
        if (_busy)
            SetStatus(wxString::Format("%s... %d%%", stage, percent));
    }

    void UploadView::OnUploadFinished(bool ok, const wxString& message)
    {
        _busy = false;
        _cancel->SetLabelText("Back");
        SetStatus(message);

        if (ok)
        {
            std::vector<wxString> paths;
            for (const Core::LogScanner::LogFile& file : _scan.Files)
                paths.push_back(file.Path);
            _onUploaded(paths);
            ClearRows();
            return;
        }

        OnSelectionChanged();
        _selectAll->Enable(!_rows.empty());
    }

    void UploadView::OnCancel()
    {
        if (_cancelled)
            *_cancelled = true;

        _busy = false;
        ClearRows();
        _onClosed();
    }

    void UploadView::SetStatus(const wxString& status)
    {
        _status->SetLabelText(status);
    }

    void UploadView::ClearRows()
    {
        _rows.clear();
        _listSizer->Clear(true);
        _list->FitInside();
    }
}
