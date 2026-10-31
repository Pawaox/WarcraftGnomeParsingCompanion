#pragma once

#include "Core/Expansion.h"
#include "Ui/SubView.h"

#include <wx/panel.h>
#include <wx/tglbtn.h>

#include <array>
#include <functional>

namespace Ui
{
    class NavBar : public wxPanel
    {
    public:
        NavBar(wxWindow* parent,
            std::function<void(Core::Expansion)> onExpansionSelected,
            std::function<void(SubView)> onSubViewSelected);

        void SetSelection(Core::Expansion expansion, SubView subView);
        void FocusExpansion(Core::Expansion expansion);

    private:
        void AddSeparator(wxSizer* sizer);
        wxToggleButton* AddSubViewButton(wxSizer* sizer, const wxString& label, SubView subView);

        std::function<void(Core::Expansion)> _onExpansionSelected;
        std::function<void(SubView)> _onSubViewSelected;
        std::array<wxBitmapToggleButton*, Core::ALL_EXPANSIONS.size()> _expansionButtons{};
        wxToggleButton* _logsButton = nullptr;
        wxToggleButton* _settingsButton = nullptr;
    };
}
