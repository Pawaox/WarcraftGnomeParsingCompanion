#include "Ui/Controls/Tab.h"

#include "Ui/Palette.h"

#include <wx/dcbuffer.h>

namespace Ui::Controls
{
    namespace
    {
        constexpr int UNDERLINE_HEIGHT = 2;
    }

    Tab::Tab(wxWindow* parent, std::function<void()> onClick)
        : _onClick(std::move(onClick))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        SetCursor(wxCursor(wxCURSOR_HAND));

        Bind(wxEVT_PAINT, &Tab::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { _hovered = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { _hovered = false; Refresh(); });
        Bind(wxEVT_LEFT_UP, [this](wxMouseEvent&) { _onClick(); });
    }

    void Tab::SetSelected(bool selected)
    {
        if (_selected == selected)
            return;
        _selected = selected;
        Refresh();
    }

    void Tab::DrawUnderline(wxDC& dc, int x, int width)
    {
        int height = FromDIP(UNDERLINE_HEIGHT);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(Palette::Current().TabActive));
        dc.DrawRectangle(x, GetClientSize().y - height, width, height);
    }

    void Tab::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();
        Paint(dc);
    }
}
