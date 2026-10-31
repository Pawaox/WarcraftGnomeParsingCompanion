#pragma once

#include <wx/string.h>

#include <array>
#include <optional>
#include <vector>

namespace Core
{
    enum class Expansion
    {
        Vanilla,
        TBC,
        WOTLK,
        Forever
    };

    inline constexpr std::array<Expansion, 4> ALL_EXPANSIONS =
    {
        Expansion::Vanilla,
        Expansion::TBC,
        Expansion::WOTLK,
        Expansion::Forever
    };

    inline constexpr Expansion DEFAULT_EXPANSION = Expansion::Forever;

    size_t ExpansionIndex(Expansion expansion);
    wxString ExpansionKey(Expansion expansion);
    wxString ExpansionName(Expansion expansion);
    std::vector<wxString> ExpansionClientFolders(Expansion expansion);
    std::optional<Expansion> ExpansionFromKey(const wxString& key);
}
