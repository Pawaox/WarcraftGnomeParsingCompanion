#pragma once

#include "Core/Settings.h"

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filepicker.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>

#include <array>
#include <functional>

namespace Ui
{
    class SettingsView : public wxScrolledWindow
    {
    public:
        SettingsView(wxWindow* parent, Core::Settings& settings,
            std::function<Core::Expansion()> currentExpansion,
            std::function<void()> onFoldersChanged);

        void Reload();

    private:
        void AddHeading(wxSizer* sizer, const wxString& title);
        wxFlexGridSizer* AddSection(wxSizer* sizer, const wxString& title);
        void AddParagraph(wxSizer* sizer, const wxString& text);
        wxDirPickerCtrl* AddFolderPicker(wxFlexGridSizer* grid, const wxString& label, int indent);
        void AddLabeledRow(wxFlexGridSizer* grid, const wxString& label, wxWindow* control);
        void AddCheckBoxRow(wxFlexGridSizer* grid, wxCheckBox* checkBox);

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
        wxCheckBox* _autoArchive = nullptr;
        wxChoice* _autoDeleteArchive = nullptr;
        wxCheckBox* _runOnStartup = nullptr;
        wxCheckBox* _closeToTray = nullptr;
        wxChoice* _theme = nullptr;
    };
}
