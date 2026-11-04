#include "Ui/Controls/TextTab.h"

#include "Ui/Fonts.h"
#include "Ui/Palette.h"

namespace Ui::Controls
{
    namespace
    {
        constexpr int PADDING_TOP = 4;
        constexpr int PADDING_SIDE = 10;
        constexpr int UNDERLINE_SPACE = 9;
        constexpr double FONT_SIZE = 13.5;
    }

    TextTab::TextTab(wxWindow* parent, const wxString& label, std::function<void()> onClick)
        : Tab(parent, std::move(onClick)),
          _label(label)
    {
        SetFont(Fonts::SemiBold(FONT_SIZE));
    }

    void TextTab::Paint(wxDC& dc)
    {
        const Palette& palette = Palette::Current();
        dc.SetFont(GetFont());
        dc.SetTextForeground(IsSelected() ? palette.TabActive : IsHovered() ? palette.TabHover : palette.TextInactive);

        int x = FromDIP(PADDING_SIDE);
        dc.DrawText(_label, x, FromDIP(PADDING_TOP));
        if (IsSelected())
            DrawUnderline(dc, x, dc.GetTextExtent(_label).x);
    }

    wxSize TextTab::DoGetBestClientSize() const
    {
        wxSize text = GetTextExtent(_label);
        return wxSize(text.x + 2 * FromDIP(PADDING_SIDE), text.y + FromDIP(PADDING_TOP) + FromDIP(UNDERLINE_SPACE));
    }
}
