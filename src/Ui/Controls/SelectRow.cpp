#include "Ui/Controls/SelectRow.h"

#include "Ui/Fonts.h"
#include "Ui/Palette.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

#include <memory>

namespace Ui::Controls
{
    namespace
    {
        constexpr int HEIGHT = 58;
        constexpr int BOX_SIZE = 28;
        constexpr int PADDING = 14;
        constexpr double CORNER_RADIUS = 7;
        constexpr double BOX_RADIUS = 5;
        constexpr double CHECK_STROKE = 3;
        constexpr double DETAIL_SIZE = 9.5;
        constexpr int HOVER_LIGHTEN = 115;
    }

    SelectRow::SelectRow(wxWindow* parent, const wxString& title, const wxString& detail, std::function<void()> onChanged)
        : _title(title),
          _detail(detail),
          _onChanged(std::move(onChanged))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        SetCursor(wxCursor(wxCURSOR_HAND));
        SetLabel(title + " " + detail);

        Bind(wxEVT_PAINT, &SelectRow::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { _hovered = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { _hovered = false; Refresh(); });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { Toggle(); });
        Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent&) { Toggle(); });
    }

    void SelectRow::SetSelected(bool selected)
    {
        if (_selected == selected)
            return;
        _selected = selected;
        Refresh();
    }

    void SelectRow::Toggle()
    {
        SetSelected(!_selected);
        _onChanged();
    }

    wxSize SelectRow::DoGetBestClientSize() const
    {
        return wxSize(-1, FromDIP(HEIGHT));
    }

    void SelectRow::OnPaint(wxPaintEvent&)
    {
        const Palette& palette = Palette::Current();
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        wxSize size = GetClientSize();
        int padding = FromDIP(PADDING);
        double boxSize = FromDIP(BOX_SIZE);
        double boxY = (size.y - boxSize) / 2.0;

        wxColour fill = _hovered ? palette.InputBackground.ChangeLightness(HOVER_LIGHTEN) : palette.InputBackground;
        wxColour border = _selected ? palette.NavBorder : _hovered ? palette.Accent : palette.InputBorder;

        if (std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc)); gc)
        {
            double line = FromDIP(1);
            double radius = FromDIP(CORNER_RADIUS);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush(border));
            gc->DrawRoundedRectangle(0, 0, size.x, size.y, radius);
            gc->SetBrush(wxBrush(fill));
            gc->DrawRoundedRectangle(line, line, size.x - 2 * line, size.y - 2 * line, radius - line);

            gc->SetBrush(wxBrush(_selected ? palette.NavBorder : palette.InputBorder));
            gc->DrawRoundedRectangle(padding, boxY, boxSize, boxSize, FromDIP(BOX_RADIUS));

            if (_selected)
            {
                double left = padding + boxSize * 0.24;
                double middle = padding + boxSize * 0.43;
                double right = padding + boxSize * 0.78;
                gc->SetPen(wxPen(*wxWHITE, FromDIP(CHECK_STROKE)));
                wxGraphicsPath check = gc->CreatePath();
                check.MoveToPoint(left, boxY + boxSize * 0.52);
                check.AddLineToPoint(middle, boxY + boxSize * 0.72);
                check.AddLineToPoint(right, boxY + boxSize * 0.3);
                gc->StrokePath(check);
            }
        }

        int textX = padding * 2 + FromDIP(BOX_SIZE);

        dc.SetFont(Fonts::SemiBold());
        dc.SetTextForeground(palette.Text);
        int titleHeight = dc.GetTextExtent(_title).y;

        wxFont detailFont = Fonts::Regular(DETAIL_SIZE);
        int detailHeight = dc.GetTextExtent(wxS("Ag")).y;
        dc.SetFont(detailFont);
        detailHeight = dc.GetTextExtent(wxS("Ag")).y;

        int textTop = (size.y - titleHeight - detailHeight) / 2;

        dc.SetFont(Fonts::SemiBold());
        dc.DrawText(_title, textX, textTop);

        dc.SetFont(detailFont);
        dc.SetTextForeground(palette.TextInactive);
        dc.DrawText(_detail, textX, textTop + titleHeight);
    }
}
