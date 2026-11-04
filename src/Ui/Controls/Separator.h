#pragma once

#include <wx/window.h>

namespace Ui::Controls
{
    class Separator : public wxWindow
    {
    public:
        Separator(wxWindow* parent, wxOrientation orientation, const wxColour& colour, int length = 0);

        bool AcceptsFocus() const override { return false; }
    };
}
