#pragma once

#include <wx/window.h>

#include <functional>

namespace Ui::Controls
{
    class SelectRow : public wxWindow
    {
    public:
        SelectRow(wxWindow* parent, const wxString& title, const wxString& detail, std::function<void()> onChanged);

        void SetSelected(bool selected);
        bool IsSelected() const { return _selected; }
        bool AcceptsFocus() const override { return false; }

    protected:
        wxSize DoGetBestClientSize() const override;

    private:
        void OnPaint(wxPaintEvent& event);
        void Toggle();

        wxString _title;
        wxString _detail;
        std::function<void()> _onChanged;
        bool _selected = false;
        bool _hovered = false;
    };
}
