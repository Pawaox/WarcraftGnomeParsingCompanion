#pragma once

#include <wx/window.h>

#include <functional>

namespace Ui::Controls
{
    class Button : public wxWindow
    {
    public:
        enum class Style
        {
            Normal,
            Primary
        };

        enum class Icon
        {
            None,
            Copy,
            Check
        };

        Button(wxWindow* parent, const wxString& label, std::function<void()> onClick, Style style = Style::Normal);

        void SetLabelText(const wxString& label);
        void SetIcon(Icon icon);
        bool Enable(bool enable = true) override;
        bool AcceptsFocus() const override { return false; }

    protected:
        wxSize DoGetBestClientSize() const override;

    private:
        void OnPaint(wxPaintEvent& event);
        void OnMouseUp(wxMouseEvent& event);

        wxString _text;
        std::function<void()> _onClick;
        Style _style;
        Icon _icon = Icon::None;
        bool _hovered = false;
        bool _pressed = false;
    };
}
