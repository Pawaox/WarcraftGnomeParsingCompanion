#pragma once

#include <wx/arrstr.h>
#include <wx/window.h>

#include <functional>

namespace Ui::Controls
{
    class Dropdown : public wxWindow
    {
    public:
        Dropdown(wxWindow* parent, const wxArrayString& items, std::function<void(int)> onSelected);

        void SetSelection(int index);
        void SetItems(const wxArrayString& items);
        int GetSelection() const { return _selection; }
        bool Enable(bool enable = true) override;
        bool AcceptsFocus() const override { return false; }

    protected:
        wxSize DoGetBestClientSize() const override;

    private:
        void OnPaint(wxPaintEvent& event);
        void ShowMenu();

        wxArrayString _items;
        std::function<void(int)> _onSelected;
        int _selection = 0;
        bool _hovered = false;
    };
}
