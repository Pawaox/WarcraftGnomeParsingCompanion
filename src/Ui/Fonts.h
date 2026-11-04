#pragma once

#include <wx/font.h>

namespace Ui::Fonts
{
    inline constexpr double BASE_SIZE = 11.0;

    void Register();
    wxFont Regular(double pointSize = BASE_SIZE);
    wxFont Italic(double pointSize = BASE_SIZE);
    wxFont SemiBold(double pointSize = BASE_SIZE);
    wxFont Bold(double pointSize = BASE_SIZE);
}
