#include "Ui/Controls/ToggleSwitch.h"

#include "Ui/Palette.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <algorithm>
#include <memory>

namespace Ui::Controls
{
    namespace
    {
        constexpr int WIDTH = 46;
        constexpr int HEIGHT = 24;
        constexpr int KNOB_INSET = 3;
        constexpr double CORNER_RADIUS = 7;
        constexpr double KNOB_CORNER_RADIUS = 4;
        constexpr double ICON_SIZE = 4.0;
        constexpr double ICON_STROKE = 2.0;
        constexpr int ANIMATION_TICK_MS = 15;
        constexpr double ANIMATION_STEP = 0.2;

        wxColour Blend(const wxColour& from, const wxColour& to, double amount)
        {
            auto channel = [amount](int start, int end) { return static_cast<unsigned char>(start + (end - start) * amount); };
            return wxColour(channel(from.Red(), to.Red()), channel(from.Green(), to.Green()), channel(from.Blue(), to.Blue()));
        }
    }

    ToggleSwitch::ToggleSwitch(wxWindow* parent, std::function<void()> onClicked)
        : _onClicked(std::move(onClicked))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        SetCursor(wxCursor(wxCURSOR_HAND));

        Bind(wxEVT_PAINT, &ToggleSwitch::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { _hovered = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { _hovered = false; Refresh(); });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { _onClicked(); });
        Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent&) { _onClicked(); });
        _animation.Bind(wxEVT_TIMER, [this](wxTimerEvent&) { OnAnimationTick(); });
    }

    void ToggleSwitch::SetValue(bool value)
    {
        if (value == _value)
            return;

        _value = value;
        if (IsShownOnScreen())
        {
            _animation.Start(ANIMATION_TICK_MS);
        }
        else
        {
            _position = _value ? 1.0 : 0.0;
            Refresh();
        }
    }

    wxSize ToggleSwitch::DoGetBestClientSize() const
    {
        return FromDIP(wxSize(WIDTH, HEIGHT));
    }

    void ToggleSwitch::OnAnimationTick()
    {
        double target = _value ? 1.0 : 0.0;
        _position = _value ? std::min(target, _position + ANIMATION_STEP) : std::max(target, _position - ANIMATION_STEP);
        if (_position == target)
            _animation.Stop();
        Refresh();
    }

    void ToggleSwitch::OnPaint(wxPaintEvent&)
    {
        const Palette& palette = Palette::Current();
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
            return;

        wxSize size = GetClientSize();
        double radius = FromDIP(CORNER_RADIUS);
        double border = FromDIP(1);
        double inset = FromDIP(KNOB_INSET);
        double knobSize = size.y - 2 * inset;
        double knobTravel = size.x - 2 * inset - knobSize;
        double knobX = inset + knobTravel * _position;
        double center = size.y / 2.0;

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(Blend(_hovered ? palette.Accent : palette.InputBorder, palette.NavBorder, _position)));
        gc->DrawRoundedRectangle(0, 0, size.x, size.y, radius);
        gc->SetBrush(wxBrush(Blend(palette.InputBackground, palette.NavBorder, _position)));
        gc->DrawRoundedRectangle(border, border, size.x - 2 * border, size.y - 2 * border, radius - border);

        double iconSize = FromDIP(ICON_SIZE);
        double iconY = center;
        wxColour iconColour = _value ? wxColour(*wxWHITE) : palette.TextInactive;
        gc->SetPen(wxPen(iconColour, FromDIP(ICON_STROKE)));
        wxGraphicsPath icon = gc->CreatePath();
        if (_value)
        {
            double iconX = center;
            icon.MoveToPoint(iconX - iconSize, iconY);
            icon.AddLineToPoint(iconX - iconSize * 0.25, iconY + iconSize * 0.75);
            icon.AddLineToPoint(iconX + iconSize, iconY - iconSize * 0.75);
        }
        else
        {
            double iconX = size.x - center;
            double half = iconSize * 0.75;
            icon.MoveToPoint(iconX - half, iconY - half);
            icon.AddLineToPoint(iconX + half, iconY + half);
            icon.MoveToPoint(iconX + half, iconY - half);
            icon.AddLineToPoint(iconX - half, iconY + half);
        }
        gc->StrokePath(icon);

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(Blend(palette.TextInactive, *wxWHITE, _position)));
        gc->DrawRoundedRectangle(knobX, inset, knobSize, knobSize, FromDIP(KNOB_CORNER_RADIUS));
    }
}
