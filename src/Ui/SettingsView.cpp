#include "Ui/SettingsView.h"

#include "Core/AppInfo.h"
#include "Core/LogArchive.h"
#include "Core/Startup.h"
#include "Core/WowFolders.h"
#include "Ui/Controls/Button.h"
#include "Ui/Fonts.h"
#include "Ui/Palette.h"

#include <wx/hyperlink.h>
#include <wx/msgdlg.h>
#include <wx/taskbar.h>
#include <wx/textctrl.h>

#include <algorithm>

namespace Ui
{
    namespace
    {
        constexpr int MARGIN = 16;
        constexpr int HEADING_MARGIN_TOP = 22;
        constexpr double HEADING_SIZE = 13.0;
        constexpr double NOTE_SIZE = 10.0;
        constexpr int LABEL_WIDTH = 200;
        constexpr int PARAGRAPH_WIDTH = 720;
        constexpr int ACTION_BUTTON_HEIGHT = 36;
        constexpr int ACTION_BUTTON_PADDING = 12;
        constexpr int YES = 0;
        constexpr int NO = 1;

        const wxArrayString YES_NO_LABELS = { "Yes", "No" };
        const wxArrayString AUTO_DELETE_ARCHIVE_LABELS =
        {
            "Never", "After 3 Days", "After 7 Days", "After 14 Days", "After 30 Days", "After 60 Days"
        };
        const wxArrayString THEME_LABELS = { "System", "Light", "Dark" };

        int YesNo(bool value)
        {
            return value ? YES : NO;
        }

        void ShowMessage(wxWindow* parent, const wxString& message, long style)
        {
            wxMessageBox(message, Core::AppInfo::NAME, wxOK | style, parent);
        }

        bool Confirm(wxWindow* parent, const wxString& message)
        {
            return wxMessageBox(message, Core::AppInfo::NAME, wxYES_NO | wxNO_DEFAULT | wxICON_WARNING, parent) == wxYES;
        }
    }

    SettingsView::SettingsView(wxWindow* parent, Core::Settings& settings, std::function<void()> onLogout,
        std::function<Core::Expansion()> currentExpansion,
        std::function<void()> onFoldersChanged)
        : wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL),
          _settings(settings),
          _currentExpansion(std::move(currentExpansion)),
          _onFoldersChanged(std::move(onFoldersChanged))
    {
        const Palette& palette = Palette::Current();
        SetBackgroundColour(palette.Background);

        auto* sizer = new wxBoxSizer(wxVERTICAL);

        wxFlexGridSizer* wow = AddSection(sizer, "World of Warcraft");
        _rootPicker = AddFolderPicker(wow, "World of Warcraft Directory");
        _rootPicker->Bind(wxEVT_DIRPICKER_CHANGED, [this](wxFileDirPickerEvent&) { OnRootChanged(); });
        for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
        {
            wxDirPickerCtrl* picker = AddFolderPicker(wow, Core::ExpansionKey(expansion) + " Directory");
            picker->Bind(wxEVT_DIRPICKER_CHANGED, [this, expansion](wxFileDirPickerEvent&) { OnExpansionFolderChanged(expansion); });
            _expansionPickers[Core::ExpansionIndex(expansion)] = picker;
        }

        wxFlexGridSizer* cleanup = AddSection(sizer, "Combatlog Cleanup");
        _autoArchive = NewYesNo([this] { OnAutoArchiveChanged(); });
        AddRow(cleanup, "Auto Archive Combat Logs", _autoArchive);
        _autoDeleteArchive = new Controls::Dropdown(this, AUTO_DELETE_ARCHIVE_LABELS, [this](int) { OnAutoDeleteArchiveChanged(); });
        AddRow(cleanup, "Auto Delete Archive", _autoDeleteArchive);

        wxFlexGridSizer* startup = AddSection(sizer, "Startup Settings");
        _runOnStartup = NewYesNo([this] { OnRunOnStartupChanged(); });
        AddRow(startup, "Run on Startup", _runOnStartup);
        _closeToTray = NewYesNo([this] { OnCloseToTrayChanged(); });
        _closeToTray->Enable(wxTaskBarIcon::IsAvailable());
        AddRow(startup, "Close to Tray", _closeToTray);

        wxFlexGridSizer* ui = AddSection(sizer, "UI Settings");
        _theme = new Controls::Dropdown(this, THEME_LABELS, [this](int) { OnThemeChanged(); });
        auto* themeRow = new wxBoxSizer(wxHORIZONTAL);
        themeRow->Add(_theme, 0, wxALIGN_CENTER_VERTICAL);
        wxStaticText* themeNote = NewText("requires restart", palette.TextInactive);
        themeNote->SetFont(Fonts::Italic(NOTE_SIZE));
        themeRow->Add(themeNote, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(12));
        AddLabel(ui, "Theme");
        ui->Add(themeRow, 0, wxALIGN_CENTER_VERTICAL);

        _actionsHeading = AddHeading(sizer, "Actions");
        auto* actions = new wxBoxSizer(wxHORIZONTAL);
        AddActionButton(actions, "Archive Combat Logs", [this] { OnArchiveNow(); });
        AddActionButton(actions, "Delete Archived Logs", [this] { OnDeleteArchivedNow(); });
        sizer->Add(actions, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));

        AddHeading(sizer, "Account");
        auto* account = new wxBoxSizer(wxHORIZONTAL);
        AddActionButton(account, "Log out", std::move(onLogout));
        sizer->Add(account, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));

        AddHeading(sizer, "Tracking");
        AddParagraph(sizer, "This app doesn't do telemetry, it doesn't use cookies, it doesn't check anything outside of the selected wow folder, except it's own settings.");
        AddParagraph(sizer, "Errors & Crashes are not sent anywhere. If this app sucks, I won't know unless you tell me.");
        AddParagraph(sizer, "If \"Run on Startup\" is on, this is written to registry on the PC. It is removed again if turned off.");
        AddParagraph(sizer, "It's also opensource, you can check the code here:");
        auto* link = new wxHyperlinkCtrl(this, wxID_ANY, Core::AppInfo::SOURCE_URL, Core::AppInfo::SOURCE_URL);
        link->SetNormalColour(palette.Link);
        link->SetVisitedColour(palette.Link);
        link->SetHoverColour(palette.LinkHover);
        sizer->Add(link, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));
        sizer->AddSpacer(FromDIP(MARGIN));

        SetSizer(sizer);
        SetScrollRate(0, FromDIP(12));
        SetActionsExpansion(_currentExpansion());
        FitInside();
    }

    void SettingsView::Reload()
    {
        _rootPicker->SetPath(_settings.WowRoot);
        for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
            _expansionPickers[Core::ExpansionIndex(expansion)]->SetPath(_settings.ExpansionFolder(expansion));

        _autoArchive->SetSelection(YesNo(_settings.AutoArchive));
        auto days = std::ranges::find(Core::AUTO_DELETE_ARCHIVE_DAYS, _settings.AutoDeleteArchiveDays);
        _autoDeleteArchive->SetSelection(static_cast<int>(std::distance(Core::AUTO_DELETE_ARCHIVE_DAYS.begin(), days)));
        _autoDeleteArchive->Enable(_settings.AutoArchive);

        _runOnStartup->SetSelection(YesNo(_settings.RunOnStartup));
        _closeToTray->SetSelection(YesNo(_settings.CloseToTray && wxTaskBarIcon::IsAvailable()));
        _theme->SetSelection(static_cast<int>(_settings.Theme));
    }

    void SettingsView::SetActionsExpansion(Core::Expansion expansion)
    {
        wxString title = Core::ExpansionKey(expansion) + " Actions";
        if (_actionsHeading->GetLabelText() == title)
            return;
        _actionsHeading->SetLabelText(title);
        Layout();
    }

    wxStaticText* SettingsView::NewText(const wxString& text, const wxColour& colour)
    {
        auto* label = new wxStaticText(this, wxID_ANY, wxEmptyString);
        label->SetLabelText(text);
        label->SetForegroundColour(colour);
        return label;
    }

    Controls::Dropdown* SettingsView::NewYesNo(std::function<void()> onChanged)
    {
        return new Controls::Dropdown(this, YES_NO_LABELS, [onChanged = std::move(onChanged)](int) { onChanged(); });
    }

    wxStaticText* SettingsView::AddHeading(wxSizer* sizer, const wxString& title)
    {
        wxStaticText* heading = NewText(title, Palette::Current().Accent);
        heading->SetFont(Fonts::Bold(HEADING_SIZE));
        sizer->AddSpacer(FromDIP(HEADING_MARGIN_TOP - MARGIN));
        sizer->Add(heading, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));
        return heading;
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
        wxStaticText* paragraph = NewText(text, Palette::Current().Text);
        paragraph->Wrap(FromDIP(PARAGRAPH_WIDTH));
        sizer->Add(paragraph, 0, wxLEFT | wxRIGHT | wxTOP, FromDIP(MARGIN));
    }

    void SettingsView::AddLabel(wxFlexGridSizer* grid, const wxString& label)
    {
        wxStaticText* text = NewText(label, Palette::Current().Text);
        text->SetMinSize(wxSize(FromDIP(LABEL_WIDTH), -1));
        grid->Add(text, 0, wxALIGN_CENTER_VERTICAL);
    }

    wxDirPickerCtrl* SettingsView::AddFolderPicker(wxFlexGridSizer* grid, const wxString& label)
    {
        const Palette& palette = Palette::Current();
        AddLabel(grid, label);

        auto* picker = new wxDirPickerCtrl(this, wxID_ANY, wxEmptyString, label, wxDefaultPosition, wxDefaultSize,
            wxDIRP_USE_TEXTCTRL | wxDIRP_DIR_MUST_EXIST);
        if (wxTextCtrl* text = picker->GetTextCtrl())
        {
            text->SetBackgroundColour(palette.InputBackground);
            text->SetForegroundColour(palette.InputText);
        }
        grid->Add(picker, 1, wxEXPAND);
        return picker;
    }

    void SettingsView::AddRow(wxFlexGridSizer* grid, const wxString& label, wxWindow* control)
    {
        AddLabel(grid, label);
        grid->Add(control, 0, wxALIGN_CENTER_VERTICAL);
    }

    Controls::Button* SettingsView::AddActionButton(wxSizer* sizer, const wxString& label, std::function<void()> onClick)
    {
        auto* button = new Controls::Button(this, label, std::move(onClick));
        button->SetMinSize(wxSize(button->GetBestSize().x + FromDIP(ACTION_BUTTON_PADDING), FromDIP(ACTION_BUTTON_HEIGHT)));
        sizer->Add(button, 0, wxRIGHT, FromDIP(10));
        return button;
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
        bool enable = _autoArchive->GetSelection() == YES;
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
                _autoArchive->SetSelection(NO);
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
        bool enable = _runOnStartup->GetSelection() == YES;
        bool succeeded = enable ? Core::Startup::Enable() : Core::Startup::Disable();
        if (!succeeded)
        {
            ShowMessage(this, enable ? "Couldn't add the app to " + Core::Startup::EntryLocation() + "."
                : "Couldn't remove the app from " + Core::Startup::EntryLocation() + ".", wxICON_ERROR);
            _runOnStartup->SetSelection(YesNo(!enable));
            return;
        }

        _settings.RunOnStartup = enable;
        _settings.Save();
    }

    void SettingsView::OnCloseToTrayChanged()
    {
        _settings.CloseToTray = _closeToTray->GetSelection() == YES;
        _settings.Save();
    }

    void SettingsView::OnThemeChanged()
    {
        int selection = _theme->GetSelection();
        if (selection < 0 || selection >= static_cast<int>(THEME_LABELS.size()))
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
