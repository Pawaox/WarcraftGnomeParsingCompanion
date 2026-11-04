#pragma once

#include <wx/dc.h>
#include <wx/window.h>

#include <functional>

namespace Ui::Controls
{
    class Tab : public wxWindow
    {
    public:
        Tab(wxWindow* parent, std::function<void()> onClick);

        void SetSelected(bool selected);
        bool AcceptsFocus() const override { return false; }

    protected:
        bool IsSelected() const { return _selected; }
        bool IsHovered() const { return _hovered; }
        virtual void Paint(wxDC& dc) = 0;
        void DrawUnderline(wxDC& dc, int x, int width);

    private:
        void OnPaint(wxPaintEvent& event);

        std::function<void()> _onClick;
        bool _selected = false;
        bool _hovered = false;
    };
}
