#pragma once

#include <wx/colour.h>

namespace Ui
{
    class Palette
    {
    public:
        static void Use(bool dark);
        static const Palette& Current();

        wxColour Background;
        wxColour NavBackground;
        wxColour NavBorder;
        wxColour Text;
        wxColour TextInactive;
        wxColour Title;
        wxColour Accent;
        wxColour TabActive;
        wxColour TabHover;
        wxColour InputBackground;
        wxColour InputText;
        wxColour InputBorder;
        wxColour Link;
        wxColour LinkHover;
    };
}
