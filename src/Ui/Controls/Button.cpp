#include "Ui/Controls/Button.h"

#include "Ui/Palette.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <memory>

namespace Ui::Controls
{
    namespace
    {
        constexpr int PADDING_SIDE = 14;
        constexpr int PADDING_VERTICAL = 6;
        constexpr double CORNER_RADIUS = 6;
        constexpr int HOVER_LIGHTEN = 115;
        constexpr int PRESS_DARKEN = 90;
    }

    Button::Button(wxWindow* parent, const wxString& label, std::function<void()> onClick, Style style)
        : _text(label),
          _onClick(std::move(onClick)),
          _style(style)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        SetCursor(wxCursor(wxCURSOR_HAND));
        SetLabel(label);

        auto press = [this](wxMouseEvent&)
        {
            _pressed = true;
            CaptureMouse();
            Refresh();
        };

        Bind(wxEVT_PAINT, &Button::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { _hovered = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { _hovered = false; Refresh(); });
        Bind(wxEVT_LEFT_DOWN, press);
        Bind(wxEVT_LEFT_DCLICK, press);
        Bind(wxEVT_LEFT_UP, &Button::OnMouseUp, this);
        Bind(wxEVT_MOUSE_CAPTURE_LOST, [this](wxMouseCaptureLostEvent&) { _pressed = false; Refresh(); });
    }

    void Button::SetLabelText(const wxString& label)
    {
        if (_text == label)
            return;
        _text = label;
        SetLabel(label);
        InvalidateBestSize();
        Refresh();
    }

    void Button::SetIcon(Icon icon)
    {
        _icon = icon;
        InvalidateBestSize();
        Refresh();
    }

    bool Button::Enable(bool enable)
    {
        if (!wxWindow::Enable(enable))
            return false;
        _hovered = false;
        _pressed = false;
        SetCursor(enable ? wxCursor(wxCURSOR_HAND) : wxNullCursor);
        Refresh();
        return true;
    }

    wxSize Button::DoGetBestClientSize() const
    {
        if (_icon != Icon::None)
        {
            int side = GetCharHeight() + 2 * FromDIP(PADDING_VERTICAL);
            return wxSize(side, side);
        }

        wxSize text = GetTextExtent(_text);
        return wxSize(text.x + 2 * FromDIP(PADDING_SIDE), text.y + 2 * FromDIP(PADDING_VERTICAL));
    }

    void Button::OnMouseUp(wxMouseEvent& event)
    {
        if (!_pressed)
            return;

        if (HasCapture())
            ReleaseMouse();

        _pressed = false;
        Refresh();

        if (GetClientRect().Contains(event.GetPosition()))
            _onClick();
    }

    void Button::OnPaint(wxPaintEvent&)
    {
        const Palette& palette = Palette::Current();
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        bool primary = _style == Style::Primary;
        wxColour fill = primary ? palette.NavBorder : palette.InputBackground;
        wxColour border = primary ? palette.NavBorder : palette.InputBorder;
        wxColour text = primary ? wxColour(*wxWHITE) : palette.Text;

        if (!IsEnabled())
        {
            fill = palette.Background;
            border = palette.InputBorder;
            text = palette.TextInactive;
        }
        else if (_pressed)
        {
            fill = fill.ChangeLightness(PRESS_DARKEN);
        }
        else if (_hovered)
        {
            fill = fill.ChangeLightness(HOVER_LIGHTEN);
            border = primary ? fill : palette.Accent;
        }

        wxSize size = GetClientSize();
        if (std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc)); gc)
        {
            double inset = FromDIP(1);
            double radius = FromDIP(CORNER_RADIUS);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(border));
            gc->DrawRoundedRectangle(0, 0, size.x, size.y, radius);
            gc->SetBrush(wxBrush(fill));
            gc->DrawRoundedRectangle(inset, inset, size.x - 2 * inset, size.y - 2 * inset, radius - inset);

            double unit = FromDIP(1);
            double centerX = size.x / 2.0;
            double centerY = size.y / 2.0;

            if (_icon == Icon::Copy)
            {
                double side = 9 * unit;
                double offset = 3 * unit;
                double left = centerX - (side + offset) / 2;
                double top = centerY - (side + offset) / 2;

                gc->SetPen(wxPen(text, FromDIP(1)));
                gc->SetBrush(wxBrush(fill));
                gc->DrawRoundedRectangle(left + offset, top, side, side, 2 * unit);
                gc->DrawRoundedRectangle(left, top + offset, side, side, 2 * unit);
            }
            else if (_icon == Icon::Check)
            {
                wxGraphicsPath check = gc->CreatePath();
                check.MoveToPoint(centerX - 5 * unit, centerY);
                check.AddLineToPoint(centerX - 1.5 * unit, centerY + 3.5 * unit);
                check.AddLineToPoint(centerX + 5 * unit, centerY - 4 * unit);

                gc->SetPen(wxPen(text, FromDIP(2)));
                gc->StrokePath(check);
            }
        }

        if (_icon != Icon::None)
            return;

        dc.SetFont(GetFont());
        dc.SetTextForeground(text);
        wxSize extent = dc.GetTextExtent(_text);
        dc.DrawText(_text, (size.x - extent.x) / 2, (size.y - extent.y) / 2);
    }
}
