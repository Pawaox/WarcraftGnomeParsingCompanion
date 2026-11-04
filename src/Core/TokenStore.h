#pragma once

#include <wx/string.h>

namespace Core::TokenStore
{
    wxString Load();
    void Save(const wxString& token);
    void Clear();
}
