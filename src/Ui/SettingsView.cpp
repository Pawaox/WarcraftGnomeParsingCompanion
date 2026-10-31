#include "Ui/SettingsView.h"

#include "Core/AppInfo.h"
#include "Core/LogArchive.h"
#include "Core/Startup.h"
#include "Core/WowFolders.h"

#include <wx/button.h>
#include <wx/hyperlink.h>
#include <wx/msgdlg.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/taskbar.h>

#include <algorithm>

namespace Ui
{
    namespace
    {
        constexpr int MARGIN = 16;
        constexpr int SUB_FOLDER_INDENT = 20;
        constexpr int PARAGRAPH_WIDTH = 600;

        const wxString AUTO_DELETE_ARCHIVE_LABELS[] =
        {
            "Never", "After 3 Days", "After 7 Days", "After 14 Days", "After 30 Days", "After 60 Days"
        };

        const wxString THEME_LABELS[] = { "System", "Light", "Dark" };

        void ShowMessage(wxWindow* parent, const wxString& message, long style)
        {
            wxMessageBox(message, Core::AppInfo::NAME, wxOK | style, parent);
        }

        bool Confirm(wxWindow* parent, const wxString& message)
        {
            return wxMessageBox(message, Core::AppInfo::NAME, wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, parent) == wxYES;
        }
    }

    SettingsView::SettingsView(wxWindow* parent, Core::Settings& settings,
        std::function<Core::Expansion()> currentExpansion,
        std::function<void()> onFoldersChanged)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
          _settings(settings),
          _currentExpansion(std::move(currentExpansion)),
          _onFoldersChanged(std::move(onFoldersChanged))
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        wxFlexGridSizer* wow = AddSection(sizer, "World of Warcraft");
        _rootPicker = AddFolderPicker(wow, "World of Warcraft Directory", 0);
        _rootPicker->Bind(wxEVT_DIRPICKER_CHANGED, [this](wxFileDirPickerEvent&) { OnRootChanged(); });
        for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
        {
            wxDirPickerCtrl* picker = AddFolderPicker(wow, Core::ExpansionKey(expansion) + " Directory", SUB_FOLDER_INDENT);
            picker->Bind(wxEVT_DIRPICKER_CHANGED, [this, expansion](wxFileDirPickerEvent&) { OnExpansionFolderChanged(expansion); });
            _expansionPickers[Core::ExpansionIndex(expansion)] = picker;
        }

        wxFlexGridSizer* cleanup = AddSection(sizer, "Combatlog Cleanup");
        _autoArchive = new wxCheckBox(this, wxID_ANY, "Auto Archive Combat Logs");
        _autoArchive->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { OnAutoArchiveChanged(); });
        AddCheckBoxRow(cleanup, _autoArchive);
        _autoDeleteArchive = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            static_cast<int>(std::size(AUTO_DELETE_ARCHIVE_LABELS)), AUTO_DELETE_ARCHIVE_LABELS);
        _autoDeleteArchive->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { OnAutoDeleteArchiveChanged(); });
        AddLabeledRow(cleanup, "Auto Delete Archive", _autoDeleteArchive);

        wxFlexGridSizer* startup = AddSection(sizer, "Startup Settings");
        _runOnStartup = new wxCheckBox(this, wxID_ANY, "Run on Startup");
        _runOnStartup->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { OnRunOnStartupChanged(); });
        AddCheckBoxRow(startup, _runOnStartup);
        _closeToTray = new wxCheckBox(this, wxID_ANY, "Close to Tray");
        _closeToTray->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { OnCloseToTrayChanged(); });
        _closeToTray->Enable(wxTaskBarIcon::IsAvailable());
        AddCheckBoxRow(startup, _closeToTray);

        wxFlexGridSizer* ui = AddSection(sizer, "UI Settings");
        _theme = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            static_cast<int>(std::size(THEME_LABELS)), THEME_LABELS);
        _theme->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { OnThemeChanged(); });
        AddLabeledRow(ui, "Theme", _theme);
        auto* themeNote = new wxStaticText(this, wxID_ANY, wxEmptyString);
        themeNote->SetLabelText("A theme change applies the next time the app starts.");
        ui->AddSpacer(0);
        ui->Add(themeNote, 0, wxALIGN_CENTER_VERTICAL);

        AddHeading(sizer, "Actions");
        AddParagraph(sizer, "These apply to the expansion selected in the bar at the top.");
        auto* actions = new wxBoxSizer(wxHORIZONTAL);
        auto* archiveButton = new wxButton(this, wxID_ANY, "Archive Combat Logs");
        archiveButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnArchiveNow(); });
        actions->Add(archiveButton, 0, wxRIGHT, FromDIP(8));
        auto* deleteButton = new wxButton(this, wxID_ANY, "Delete Archived Logs");
        deleteButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnDeleteArchivedNow(); });
        actions->Add(deleteButton);
        sizer->Add(actions, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));

        AddHeading(sizer, "Tracking");
        AddParagraph(sizer, "This app doesn't do telemetry, it doesn't use cookies, it doesn't check anything outside of the selected wow folder, except it's own settings.");
        AddParagraph(sizer, "Errors & Crashes are not sent anywhere. If this app sucks, I won't know unless you tell me.");
        AddParagraph(sizer, "Run on Startup is the one thing that writes outside the app's own folder: it adds an entry to "
            + Core::Startup::EntryLocation() + ", and removes it again when you turn it off.");
        AddParagraph(sizer, "It's also opensource, you can check the code here (or ask your friendly neighbour AI to check it for you):");
        wxString sourceUrl = Core::AppInfo::SOURCE_URL;
        if (sourceUrl.empty())
            AddParagraph(sizer, "(link coming soon)");
        else
            sizer->Add(new wxHyperlinkCtrl(this, wxID_ANY, sourceUrl, sourceUrl), 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));

        AddParagraph(sizer, wxString("Version ") + Core::AppInfo::VERSION);
        sizer->AddSpacer(FromDIP(MARGIN));

        SetSizer(sizer);
        SetScrollRate(0, FromDIP(12));
        FitInside();
    }

    void SettingsView::Reload()
    {
        _rootPicker->SetPath(_settings.WowRoot);
        for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
            _expansionPickers[Core::ExpansionIndex(expansion)]->SetPath(_settings.ExpansionFolder(expansion));

        _autoArchive->SetValue(_settings.AutoArchive);
        auto days = std::ranges::find(Core::AUTO_DELETE_ARCHIVE_DAYS, _settings.AutoDeleteArchiveDays);
        _autoDeleteArchive->SetSelection(static_cast<int>(std::distance(Core::AUTO_DELETE_ARCHIVE_DAYS.begin(), days)));
        _autoDeleteArchive->Enable(_settings.AutoArchive);

        _runOnStartup->SetValue(_settings.RunOnStartup);
        _closeToTray->SetValue(_settings.CloseToTray && wxTaskBarIcon::IsAvailable());
        _theme->SetSelection(static_cast<int>(_settings.Theme));
    }

    void SettingsView::AddHeading(wxSizer* sizer, const wxString& title)
    {
        auto* heading = new wxStaticText(this, wxID_ANY, wxEmptyString);
        heading->SetLabelText(title);
        heading->SetFont(GetFont().Bold().Scaled(1.15f));
        sizer->Add(heading, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));
        sizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN / 2));
    }

    wxFlexGridSizer* SettingsView::AddSection(wxSizer* sizer, const wxString& title)
    {
        AddHeading(sizer, title);
        auto* grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(12));
        grid->AddGrowableCol(1);
        sizer->Add(grid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));
        return grid;
    }

    void SettingsView::AddParagraph(wxSizer* sizer, const wxString& text)
    {
        auto* paragraph = new wxStaticText(this, wxID_ANY, wxEmptyString);
        paragraph->SetLabelText(text);
        paragraph->Wrap(FromDIP(PARAGRAPH_WIDTH));
        sizer->Add(paragraph, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));
    }

    wxDirPickerCtrl* SettingsView::AddFolderPicker(wxFlexGridSizer* grid, const wxString& label, int indent)
    {
        auto* text = new wxStaticText(this, wxID_ANY, wxEmptyString);
        text->SetLabelText(label);
        grid->Add(text, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(indent));

        auto* picker = new wxDirPickerCtrl(this, wxID_ANY, wxEmptyString, label, wxDefaultPosition, wxDefaultSize,
            wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST);
        grid->Add(picker, 1, wxEXPAND);
        return picker;
    }

    void SettingsView::AddLabeledRow(wxFlexGridSizer* grid, const wxString& label, wxWindow* control)
    {
        auto* text = new wxStaticText(this, wxID_ANY, wxEmptyString);
        text->SetLabelText(label);
        grid->Add(text, 0, wxALIGN_CENTER_VERTICAL);
        grid->Add(control, 0, wxALIGN_CENTER_VERTICAL);
    }

    void SettingsView::AddCheckBoxRow(wxFlexGridSizer* grid, wxCheckBox* checkBox)
    {
        grid->Add(checkBox, 0, wxALIGN_CENTER_VERTICAL);
        grid->AddSpacer(0);
    }

    void SettingsView::OnRootChanged()
    {
        Core::WowFolders::ChangeRoot(_settings, _rootPicker->GetPath());
        _settings.Save();

        for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
            _expansionPickers[Core::ExpansionIndex(expansion)]->SetPath(_settings.ExpansionFolder(expansion));

        _onFoldersChanged();
    }

    void SettingsView::OnExpansionFolderChanged(Core::Expansion expansion)
    {
        _settings.ExpansionFolder(expansion) = _expansionPickers[Core::ExpansionIndex(expansion)]->GetPath();
        _settings.Save();
        _onFoldersChanged();
    }

    void SettingsView::OnAutoArchiveChanged()
    {
        bool enable = _autoArchive->GetValue();
        if (enable)
        {
            wxString problems;
            for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
            {
                const wxString& folder = _settings.ExpansionFolder(expansion);
                if (!folder.empty() && !Core::LogArchive::PrepareArchiveDirectory(expansion, folder))
                    problems += "\n" + Core::ExpansionKey(expansion) + ": " + folder;
            }

            if (!problems.empty())
            {
                ShowMessage(this, "Auto Archive can't be turned on. Neither the Logs\\_archive folder nor an ArchivedLogs folder next to the app can be written to for:\n"
                    + problems, wxICON_ERROR);
                _autoArchive->SetValue(false);
                return;
            }
        }

        _settings.AutoArchive = enable;
        _settings.Save();
        _autoDeleteArchive->Enable(enable);
    }

    void SettingsView::OnAutoDeleteArchiveChanged()
    {
        int selection = _autoDeleteArchive->GetSelection();
        if (selection < 0 || selection >= static_cast<int>(Core::AUTO_DELETE_ARCHIVE_DAYS.size()))
            return;

        _settings.AutoDeleteArchiveDays = Core::AUTO_DELETE_ARCHIVE_DAYS[selection];
        _settings.Save();
    }

    void SettingsView::OnRunOnStartupChanged()
    {
        bool enable = _runOnStartup->GetValue();
        bool succeeded = enable ? Core::Startup::Enable() : Core::Startup::Disable();
        if (!succeeded)
        {
            ShowMessage(this, enable ? "Couldn't add the app to " + Core::Startup::EntryLocation() + "."
                : "Couldn't remove the app from " + Core::Startup::EntryLocation() + ".", wxICON_ERROR);
            _runOnStartup->SetValue(!enable);
            return;
        }

        _settings.RunOnStartup = enable;
        _settings.Save();
    }

    void SettingsView::OnCloseToTrayChanged()
    {
        _settings.CloseToTray = _closeToTray->GetValue();
        _settings.Save();
    }

    void SettingsView::OnThemeChanged()
    {
        int selection = _theme->GetSelection();
        if (selection < 0 || selection >= static_cast<int>(std::size(THEME_LABELS)))
            return;

        _settings.Theme = static_cast<Core::ThemeMode>(selection);
        _settings.Save();
    }

    void SettingsView::OnArchiveNow()
    {
        Core::Expansion expansion = _currentExpansion();
        const wxString& folder = _settings.ExpansionFolder(expansion);
        if (folder.empty())
        {
            ShowMessage(this, "Set the " + Core::ExpansionKey(expansion) + " Directory first.", wxICON_INFORMATION);
            return;
        }

        if (!Confirm(this, "Move all " + Core::ExpansionName(expansion) + " combat logs in this folder to the archive?\n\n"
            + Core::WowFolders::LogsDirectory(folder)))
            return;

        std::optional<Core::LogArchive::ArchiveResult> result = Core::LogArchive::ArchiveAll(expansion, folder);
        if (!result)
        {
            ShowMessage(this, "Neither the Logs\\_archive folder nor an ArchivedLogs folder next to the app can be written to.", wxICON_ERROR);
            return;
        }

        wxString message = wxString::Format("Archived %d combat log(s).", result->Moved);
        if (result->InUse > 0)
            message += wxString::Format("\n%d skipped because World of Warcraft is still writing to them.", result->InUse);
        if (result->Failed > 0)
            message += wxString::Format("\n%d couldn't be moved.", result->Failed);
        ShowMessage(this, message, result->Failed > 0 ? wxICON_WARNING : wxICON_INFORMATION);
    }

    void SettingsView::OnDeleteArchivedNow()
    {
        Core::Expansion expansion = _currentExpansion();
        const wxString& folder = _settings.ExpansionFolder(expansion);
        if (folder.empty())
        {
            ShowMessage(this, "Set the " + Core::ExpansionKey(expansion) + " Directory first.", wxICON_INFORMATION);
            return;
        }

        if (!Confirm(this, "Delete all archived " + Core::ExpansionName(expansion) + " combat logs? This can't be undone."))
            return;

        int deleted = Core::LogArchive::DeleteArchived(expansion, folder);
        ShowMessage(this, wxString::Format("Deleted %d archived combat log(s).", deleted), wxICON_INFORMATION);
    }
}
