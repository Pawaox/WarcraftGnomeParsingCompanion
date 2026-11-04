#include "Ui/Fonts.h"

#include "resources/NunitoSans_Bold_ttf.h"
#include "resources/NunitoSans_Italic_ttf.h"
#include "resources/NunitoSans_Regular_ttf.h"
#include "resources/NunitoSans_SemiBold_ttf.h"

#include <wx/settings.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

namespace Ui::Fonts
{
    namespace
    {
        const wxString FAMILY = "Nunito Sans";
        const wxString SEMIBOLD_FAMILY = "Nunito Sans SemiBold";

        bool registered = false;

#ifdef __WXMSW__
        template <size_t N>
        bool AddMemoryFont(const unsigned char (&data)[N])
        {
            DWORD count = 0;
            return ::AddFontMemResourceEx(const_cast<unsigned char*>(data), static_cast<DWORD>(N), nullptr, &count) != nullptr;
        }
#endif

        wxFont Make(double pointSize, const wxString& face, bool bold, bool italic, wxFontWeight fallbackWeight)
        {
            if (!registered)
            {
                wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
                font.SetFractionalPointSize(pointSize);
                font.SetWeight(fallbackWeight);
                if (italic)
                    font.SetStyle(wxFONTSTYLE_ITALIC);
                return font;
            }

            wxFontInfo info(pointSize);
            info.FaceName(face);
            if (bold)
                info.Bold();
            if (italic)
                info.Italic();
            return wxFont(info);
        }
    }

    void Register()
    {
#ifdef __WXMSW__
        registered = AddMemoryFont(NunitoSans_Regular_ttf)
            && AddMemoryFont(NunitoSans_Italic_ttf)
            && AddMemoryFont(NunitoSans_SemiBold_ttf)
            && AddMemoryFont(NunitoSans_Bold_ttf);
#endif
    }

    wxFont Regular(double pointSize)
    {
        return Make(pointSize, FAMILY, false, false, wxFONTWEIGHT_NORMAL);
    }

    wxFont Italic(double pointSize)
    {
        return Make(pointSize, FAMILY, false, true, wxFONTWEIGHT_NORMAL);
    }

    wxFont SemiBold(double pointSize)
    {
        return Make(pointSize, SEMIBOLD_FAMILY, false, false, wxFONTWEIGHT_SEMIBOLD);
    }

    wxFont Bold(double pointSize)
    {
        return Make(pointSize, FAMILY, true, false, wxFONTWEIGHT_BOLD);
    }
}
