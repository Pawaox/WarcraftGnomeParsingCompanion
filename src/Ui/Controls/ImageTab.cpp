#include "Ui/Controls/ImageTab.h"

#include <wx/image.h>
#include <wx/vector.h>

namespace Ui::Controls
{
    namespace
    {
        constexpr int PADDING_TOP = 4;
        constexpr int PADDING_SIDE = 4;
        constexpr int UNDERLINE_SPACE = 9;
        constexpr int DIMMED_OPACITY_PERCENT = 45;

        wxBitmapBundle Dimmed(const wxBitmapBundle& icon)
        {
            wxSize size = icon.GetDefaultSize();
            wxVector<wxBitmap> bitmaps;
            for (int scale : { 1, 2 })
            {
                wxImage image = icon.GetBitmap(size * scale).ConvertToImage().ConvertToGreyscale();
                if (!image.HasAlpha())
                    image.InitAlpha();

                unsigned char* alpha = image.GetAlpha();
                for (int i = 0, count = image.GetWidth() * image.GetHeight(); i < count; ++i)
                    alpha[i] = static_cast<unsigned char>(alpha[i] * DIMMED_OPACITY_PERCENT / 100);

                bitmaps.push_back(wxBitmap(image));
            }
            return wxBitmapBundle::FromBitmaps(bitmaps);
        }
    }

    ImageTab::ImageTab(wxWindow* parent, const wxBitmapBundle& icon, const wxString& tooltip, std::function<void()> onClick)
        : Tab(parent, std::move(onClick)),
          _icon(icon),
          _dimmedIcon(Dimmed(icon))
    {
        SetToolTip(tooltip);
    }

    void ImageTab::Paint(wxDC& dc)
    {
        const wxBitmapBundle& icon = IsSelected() || IsHovered() ? _icon : _dimmedIcon;
        wxBitmap bitmap = icon.GetBitmapFor(this);
        int width = bitmap.GetLogicalWidth();
        int x = (GetClientSize().x - width) / 2;

        dc.DrawBitmap(bitmap, x, FromDIP(PADDING_TOP), true);
        if (IsSelected())
            DrawUnderline(dc, x, width);
    }

    wxSize ImageTab::DoGetBestClientSize() const
    {
        wxSize icon = _icon.GetPreferredLogicalSizeFor(this);
        return wxSize(icon.x + 2 * FromDIP(PADDING_SIDE), icon.y + FromDIP(PADDING_TOP) + FromDIP(UNDERLINE_SPACE));
    }
}
