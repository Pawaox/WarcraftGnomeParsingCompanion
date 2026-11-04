#pragma once

#include "Core/Settings.h"
#include "Ui/Controls/Dropdown.h"

#include "Ui/Controls/Button.h"
#include <wx/filepicker.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <array>
#include <functional>

namespace Ui
{
    class SettingsView : public wxScrolledWindow
    {
    public:
        SettingsView(wxWindow* parent, Core::Settings& settings, std::function<void()> onLogout,
            std::function<Core::Expansion()> currentExpansion,
            std::function<void()> onFoldersChanged);

        void Reload();
        void SetActionsExpansion(Core::Expansion expansion);

    private:
        wxStaticText* NewText(const wxString& text, const wxColour& colour);
        Controls::Dropdown* NewYesNo(std::function<void()> onChanged);
        wxStaticText* AddHeading(wxSizer* sizer, const wxString& title);
        wxFlexGridSizer* AddSection(wxSizer* sizer, const wxString& title);
        void AddParagraph(wxSizer* sizer, const wxString& text);
        void AddLabel(wxFlexGridSizer* grid, const wxString& label);
        wxDirPickerCtrl* AddFolderPicker(wxFlexGridSizer* grid, const wxString& label);
        void AddRow(wxFlexGridSizer* grid, const wxString& label, wxWindow* control);
        Controls::Button* AddActionButton(wxSizer* sizer, const wxString& label, std::function<void()> onClick);

        void OnRootChanged();
        void OnExpansionFolderChanged(Core::Expansion expansion);
        void OnAutoArchiveChanged();
        void OnAutoDeleteArchiveChanged();
        void OnRunOnStartupChanged();
        void OnCloseToTrayChanged();
        void OnThemeChanged();
        void OnArchiveNow();
        void OnDeleteArchivedNow();

        Core::Settings& _settings;
        std::function<Core::Expansion()> _currentExpansion;
        std::function<void()> _onFoldersChanged;

        wxDirPickerCtrl* _rootPicker = nullptr;
        std::array<wxDirPickerCtrl*, Core::ALL_EXPANSIONS.size()> _expansionPickers{};
        Controls::Dropdown* _autoArchive = nullptr;
        Controls::Dropdown* _autoDeleteArchive = nullptr;
        Controls::Dropdown* _runOnStartup = nullptr;
        Controls::Dropdown* _closeToTray = nullptr;
        Controls::Dropdown* _theme = nullptr;
        wxStaticText* _actionsHeading = nullptr;
    };
}
