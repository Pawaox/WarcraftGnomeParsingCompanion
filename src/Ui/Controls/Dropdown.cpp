#include "Ui/Controls/Dropdown.h"

#include "Ui/Palette.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/menu.h>

#include <algorithm>
#include <memory>

namespace Ui::Controls
{
    namespace
    {
        constexpr int PADDING_SIDE = 9;
        constexpr int PADDING_VERTICAL = 5;
        constexpr int ARROW_AREA = 26;
        constexpr int ARROW_WIDTH = 8;
        constexpr int ARROW_HEIGHT = 4;
        constexpr int MIN_WIDTH = 90;
        constexpr double CORNER_RADIUS = 3;
        constexpr int FIRST_MENU_ID = wxID_HIGHEST + 100;
    }

    Dropdown::Dropdown(wxWindow* parent, const wxArrayString& items, std::function<void(int)> onSelected)
        : _items(items),
          _onSelected(std::move(onSelected))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        SetCursor(wxCursor(wxCURSOR_HAND));

        Bind(wxEVT_PAINT, &Dropdown::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { _hovered = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { _hovered = false; Refresh(); });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { ShowMenu(); });
    }

    void Dropdown::SetSelection(int index)
    {
        if (index < 0 || index >= static_cast<int>(_items.size()) || index == _selection)
            return;
        _selection = index;
        Refresh();
    }

    void Dropdown::SetItems(const wxArrayString& items)
    {
        _items = items;
        _selection = 0;
        InvalidateBestSize();
        Refresh();
    }

    bool Dropdown::Enable(bool enable)
    {
        if (!wxWindow::Enable(enable))
            return false;
        _hovered = false;
        Refresh();
        return true;
    }

    wxSize Dropdown::DoGetBestClientSize() const
    {
        int widest = 0;
        for (const wxString& item : _items)
            widest = std::max(widest, GetTextExtent(item).x);

        int width = std::max(widest + FromDIP(PADDING_SIDE) + FromDIP(ARROW_AREA), FromDIP(MIN_WIDTH));
        return wxSize(width, GetCharHeight() + 2 * FromDIP(PADDING_VERTICAL));
    }

    void Dropdown::OnPaint(wxPaintEvent&)
    {
        const Palette& palette = Palette::Current();
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();

        wxSize size = GetClientSize();
        wxColour text = IsEnabled() ? palette.InputText : palette.TextInactive;

        if (std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc)); gc)
        {
            gc->SetPen(wxPen(_hovered ? palette.Accent : palette.InputBorder, FromDIP(1)));
            gc->SetBrush(wxBrush(palette.InputBackground));
            gc->DrawRoundedRectangle(0.5, 0.5, size.x - 1, size.y - 1, FromDIP(CORNER_RADIUS));

            double arrowX = size.x - FromDIP(ARROW_AREA) / 2.0;
            double arrowY = size.y / 2.0;
            double halfWidth = FromDIP(ARROW_WIDTH) / 2.0;
            double halfHeight = FromDIP(ARROW_HEIGHT) / 2.0;
            gc->SetPen(wxPen(text, FromDIP(2)));
            wxGraphicsPath arrow = gc->CreatePath();
            arrow.MoveToPoint(arrowX - halfWidth, arrowY - halfHeight);
            arrow.AddLineToPoint(arrowX, arrowY + halfHeight);
            arrow.AddLineToPoint(arrowX + halfWidth, arrowY - halfHeight);
            gc->StrokePath(arrow);
        }

        if (_selection >= 0 && _selection < static_cast<int>(_items.size()))
        {
            dc.SetFont(GetFont());
            dc.SetTextForeground(text);
            dc.DrawText(_items[_selection], FromDIP(PADDING_SIDE), (size.y - GetCharHeight()) / 2);
        }
    }

    void Dropdown::ShowMenu()
    {
        wxMenu menu;
        for (size_t i = 0; i < _items.size(); ++i)
        {
            wxMenuItem* item = menu.AppendRadioItem(FIRST_MENU_ID + static_cast<int>(i), _items[i]);
            item->Check(static_cast<int>(i) == _selection);
        }

        int chosen = GetPopupMenuSelectionFromUser(menu, wxPoint(0, GetClientSize().y));
        if (chosen == wxID_NONE)
            return;

        int index = chosen - FIRST_MENU_ID;
        if (index == _selection)
            return;

        SetSelection(index);
        _onSelected(index);
    }
}
