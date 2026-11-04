#pragma once

#include "Core/Expansion.h"
#include "Ui/Controls/ImageTab.h"
#include "Ui/Controls/TextTab.h"
#include "Ui/SubView.h"

#include <wx/panel.h>
#include <wx/sizer.h>

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

    private:
        void AddSeparator(wxSizer* sizer);

        std::function<void(Core::Expansion)> _onExpansionSelected;
        std::function<void(SubView)> _onSubViewSelected;
        std::array<Controls::ImageTab*, Core::ALL_EXPANSIONS.size()> _expansionTabs{};
        Controls::TextTab* _logsTab = nullptr;
        Controls::TextTab* _settingsTab = nullptr;
    };
}
