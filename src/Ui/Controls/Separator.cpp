#include "Ui/Controls/Separator.h"

namespace Ui::Controls
{
    Separator::Separator(wxWindow* parent, wxOrientation orientation, const wxColour& colour, int length)
        : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    {
        SetBackgroundColour(colour);

        int thickness = FromDIP(1);
        int scaledLength = length > 0 ? FromDIP(length) : -1;
        SetMinSize(orientation == wxHORIZONTAL ? wxSize(scaledLength, thickness) : wxSize(thickness, scaledLength));
    }
}
