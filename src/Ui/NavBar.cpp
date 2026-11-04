#include "Ui/NavBar.h"

#include "Core/AppInfo.h"
#include "Ui/Controls/Separator.h"
#include "Ui/Fonts.h"
#include "Ui/Images.h"
#include "Ui/Palette.h"

#include <wx/statbmp.h>
#include <wx/stattext.h>

namespace Ui
{
    namespace
    {
        constexpr int ICON_SIZE = 32;
        constexpr int SEPARATOR_LENGTH = 24;
        constexpr int SEPARATOR_MARGIN = 16;
        constexpr int TAB_GAP = 6;
        constexpr int EDGE_MARGIN = 12;
        constexpr double VERSION_FONT_SIZE = 9.0;
    }

    NavBar::NavBar(wxWindow* parent,
        std::function<void(Core::Expansion)> onExpansionSelected,
        std::function<void(SubView)> onSubViewSelected)
        : wxPanel(parent),
          _onExpansionSelected(std::move(onExpansionSelected)),
          _onSubViewSelected(std::move(onSubViewSelected))
    {
        SetBackgroundColour(Palette::Current().NavBackground);

        auto* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->AddSpacer(FromDIP(EDGE_MARGIN));
        sizer->Add(new wxStaticBitmap(this, wxID_ANY, Images::Logo(ICON_SIZE)), 0, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, FromDIP(8));
        AddSeparator(sizer);

        for (Core::Expansion expansion : Core::ALL_EXPANSIONS)
        {
            auto* tab = new Controls::ImageTab(this, Images::ExpansionIcon(expansion, ICON_SIZE), Core::ExpansionName(expansion),
                [this, expansion] { _onExpansionSelected(expansion); });
            sizer->Add(tab, 0, wxALIGN_BOTTOM | wxRIGHT, FromDIP(TAB_GAP));
            _expansionTabs[Core::ExpansionIndex(expansion)] = tab;
        }

        AddSeparator(sizer);
        _logsTab = new Controls::TextTab(this, "Logs", [this] { _onSubViewSelected(SubView::Logs); });
        sizer->Add(_logsTab, 0, wxALIGN_BOTTOM | wxRIGHT, FromDIP(TAB_GAP));
        _settingsTab = new Controls::TextTab(this, "Settings", [this] { _onSubViewSelected(SubView::Settings); });
        sizer->Add(_settingsTab, 0, wxALIGN_BOTTOM);

        sizer->AddStretchSpacer();
        auto* version = new wxStaticText(this, wxID_ANY, wxString("v") + Core::AppInfo::VERSION);
        version->SetForegroundColour(Palette::Current().Accent);
        version->SetFont(Fonts::Italic(VERSION_FONT_SIZE));
        sizer->Add(version, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(EDGE_MARGIN + 4));

        SetSizer(sizer);
    }

    void NavBar::SetSelection(Core::Expansion expansion, SubView subView)
    {
        for (Core::Expansion candidate : Core::ALL_EXPANSIONS)
            _expansionTabs[Core::ExpansionIndex(candidate)]->SetSelected(candidate == expansion);

        _logsTab->SetSelected(subView == SubView::Logs);
        _settingsTab->SetSelected(subView == SubView::Settings);
    }

    void NavBar::AddSeparator(wxSizer* sizer)
    {
        sizer->Add(new Controls::Separator(this, wxVERTICAL, Palette::Current().NavBorder, SEPARATOR_LENGTH),
            0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, FromDIP(SEPARATOR_MARGIN));
    }
}
