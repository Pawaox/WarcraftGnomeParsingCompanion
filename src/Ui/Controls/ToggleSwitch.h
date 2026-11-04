#pragma once

#include <wx/timer.h>
#include <wx/window.h>

#include <functional>

namespace Ui::Controls
{
    class ToggleSwitch : public wxWindow
    {
    public:
        ToggleSwitch(wxWindow* parent, std::function<void()> onClicked);

        void SetValue(bool value);
        bool GetValue() const { return _value; }
        bool AcceptsFocus() const override { return false; }

    protected:
        wxSize DoGetBestClientSize() const override;

    private:
        void OnPaint(wxPaintEvent& event);
        void OnAnimationTick();

        std::function<void()> _onClicked;
        wxTimer _animation;
        bool _value = false;
        bool _hovered = false;
        double _position = 0.0;
    };
}
