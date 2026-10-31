#include "Ui/Images.h"

#include "resources/forever_png.h"
#include "resources/logo_png.h"
#include "resources/tbc_png.h"
#include "resources/vanilla_png.h"
#include "resources/wotlk_png.h"

#include <wx/bitmap.h>
#include <wx/icon.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/vector.h>

namespace Ui::Images
{
    namespace
    {
        template <size_t N>
        wxImage LoadPng(const unsigned char (&data)[N])
        {
            wxMemoryInputStream stream(data, N);
            return wxImage(stream, wxBITMAP_TYPE_PNG);
        }

        template <size_t N>
        wxBitmapBundle Scaled(const unsigned char (&data)[N], int size)
        {
            wxImage image = LoadPng(data);
            wxVector<wxBitmap> bitmaps;
            for (int scale : { 1, 2 })
                bitmaps.push_back(wxBitmap(image.Scale(size * scale, size * scale, wxIMAGE_QUALITY_HIGH)));
            return wxBitmapBundle::FromBitmaps(bitmaps);
        }
    }

    wxBitmapBundle Logo(int size)
    {
        return Scaled(logo_png, size);
    }

    wxBitmapBundle ExpansionIcon(Core::Expansion expansion, int size)
    {
        switch (expansion)
        {
            case Core::Expansion::Vanilla: return Scaled(vanilla_png, size);
            case Core::Expansion::TBC: return Scaled(tbc_png, size);
            case Core::Expansion::WOTLK: return Scaled(wotlk_png, size);
            case Core::Expansion::Forever: return Scaled(forever_png, size);
        }
        return {};
    }

    wxIconBundle AppIcons()
    {
        wxImage image = LoadPng(logo_png);
        wxIconBundle icons;
        for (int size : { 16, 24, 32, 48, 64, 128 })
        {
            wxIcon icon;
            icon.CopyFromBitmap(wxBitmap(image.Scale(size, size, wxIMAGE_QUALITY_HIGH)));
            icons.AddIcon(icon);
        }
        return icons;
    }
}
