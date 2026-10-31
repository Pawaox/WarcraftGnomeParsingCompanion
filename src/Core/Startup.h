#pragma once

#include <wx/string.h>

namespace Core::Startup
{
    bool Enable();
    bool Disable();
    wxString EntryLocation();
}
