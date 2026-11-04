#include "Ui/Palette.h"

namespace Ui
{
    namespace
    {
        const Palette DARK =
        {
            .Background = wxColour(0x0A, 0x0F, 0x1A),
            .NavBackground = wxColour(0x08, 0x0D, 0x18),
            .NavBorder = wxColour(0x6D, 0x5B, 0xD0),
            .Text = wxColour(0xF0, 0xF4, 0xF7),
            .TextInactive = wxColour(0x8E, 0x9B, 0xB0),
            .Title = wxColour(0xB9, 0xA2, 0xFF),
            .Accent = wxColour(0x98, 0xDC, 0xE6),
            .TabActive = wxColour(0xC9, 0xB8, 0xFF),
            .TabHover = wxColour(0xF8, 0xFB, 0xFD),
            .InputBackground = wxColour(0x18, 0x24, 0x3A),
            .InputText = wxColour(0x7F, 0xE3, 0xF0),
            .InputBorder = wxColour(0x34, 0x48, 0x6A),
            .Link = wxColour(0x67, 0xD8, 0xEF),
            .LinkHover = wxColour(0xB9, 0xA2, 0xFF)
        };

        const Palette LIGHT =
        {
            .Background = wxColour(0xF3, 0xF5, 0xF9),
            .NavBackground = wxColour(0xFF, 0xFF, 0xFF),
            .NavBorder = wxColour(0x6D, 0x5B, 0xD0),
            .Text = wxColour(0x1B, 0x24, 0x33),
            .TextInactive = wxColour(0x5F, 0x6B, 0x7E),
            .Title = wxColour(0x5B, 0x45, 0xC9),
            .Accent = wxColour(0x1C, 0x7F, 0x91),
            .TabActive = wxColour(0x5B, 0x45, 0xC9),
            .TabHover = wxColour(0x1B, 0x24, 0x33),
            .InputBackground = wxColour(0xFF, 0xFF, 0xFF),
            .InputText = wxColour(0x1B, 0x24, 0x33),
            .InputBorder = wxColour(0xC5, 0xCE, 0xDB),
            .Link = wxColour(0x0E, 0x74, 0x90),
            .LinkHover = wxColour(0x5B, 0x45, 0xC9)
        };

        const Palette* current = &DARK;
    }

    void Palette::Use(bool dark)
    {
        current = dark ? &DARK : &LIGHT;
    }

    const Palette& Palette::Current()
    {
        return *current;
    }
}
