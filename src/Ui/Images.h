#pragma once

#include "Core/Expansion.h"

#include <wx/bmpbndl.h>
#include <wx/iconbndl.h>

namespace Ui::Images
{
    wxBitmapBundle Logo(int size);
    wxBitmapBundle ExpansionIcon(Core::Expansion expansion, int size);
    wxIconBundle AppIcons();
}
