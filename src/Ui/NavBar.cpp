#include "Ui/NavBar.h"

#include "Ui/Images.h"

#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>

namespace Ui
{
    namespace
    {
        constexpr int ICON_SIZE = 32;
    }

    NavBar::NavBar(wxWindow* parent,
        std::function<void(Core::Expansion)> onExpansionSelected,
        std::function<void(SubView)> onSubViewSelected)
        : wxPanel(parent),
          _onExpansionSelected(std::move(onExpansionSelected)),
          _onSubViewSelected(std::move(onSubViewSelected))
    {
        auto* sizer = new wxBoxSizer(wxHORIZONTAL);

        sizer->Add(new wxStaticBitmap(this, wxID_ANY, Images::Logo(ICON_SIZE)), 0, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(6));
        AddSeparator(sizer);

        for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
        {
            auto* button = new wxBitmapToggleButton(this, wxID_ANY, Images::ExpansionIcon(expansion, ICON_SIZE));
            button->SetToolTip(Core::ExpansionName(expansion));
            button->Bind(wxEVT_TOGGLEBUTTON, [this, expansion](wxCommandEvent&) { _onExpansionSelected(expansion); });
            sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
            _expansionButtons[Core::ExpansionIndex(expansion)] = button;
        }

        AddSeparator(sizer);
        _logsButton = AddSubViewButton(sizer, "Logs", SubView::Logs);
        _settingsButton = AddSubViewButton(sizer, "Settings", SubView::Settings);

        SetSizer(sizer);
    }

    void NavBar::SetSelection(Core::Expansion expansion, SubView subView)
    {
        for (Core::Expansion candidate : Core::ALL_EXPANSIONS)
            _expansionButtons[Core::ExpansionIndex(candidate)]->SetValue(candidate == expansion);

        _logsButton->SetValue(subView == SubView::Logs);
        _settingsButton->SetValue(subView == SubView::Settings);
    }

    void NavBar::FocusExpansion(Core::Expansion expansion)
    {
        _expansionButtons[Core::ExpansionIndex(expansion)]->SetFocus();
    }

    void NavBar::AddSeparator(wxSizer* sizer)
    {
        sizer->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL),
            0, wxEXPAND | wxTOP | wxBOTTOM | wxLEFT | wxRIGHT, FromDIP(6));
    }

    wxToggleButton* NavBar::AddSubViewButton(wxSizer* sizer, const wxString& label, SubView subView)
    {
        auto* button = new wxToggleButton(this, wxID_ANY, label);
        button->Bind(wxEVT_TOGGLEBUTTON, [this, subView](wxCommandEvent&) { _onSubViewSelected(subView); });
        sizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
        return button;
    }
}
