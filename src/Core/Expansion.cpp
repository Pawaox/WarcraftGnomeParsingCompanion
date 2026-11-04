#include "Core/Expansion.h"

namespace Core
{
    size_t ExpansionIndex(Expansion expansion)
    {
        return static_cast<size_t>(expansion);
    }

    wxString ExpansionKey(Expansion expansion)
    {
        switch (expansion)
        {
            case Expansion::Vanilla: return "Vanilla";
            case Expansion::TBC: return "TBC";
            case Expansion::WOTLK: return "WOTLK";
            case Expansion::Forever: return "Forever";
        }
        return wxEmptyString;
    }

    wxString ExpansionName(Expansion expansion)
    {
        switch (expansion)
        {
            case Expansion::Vanilla: return "Vanilla";
            case Expansion::TBC: return "The Burning Crusade";
            case Expansion::WOTLK: return "Wrath of the Lich King";
            case Expansion::Forever: return "Forever";
        }
        return wxEmptyString;
    }

    std::vector<wxString> ExpansionClientFolders(Expansion expansion)
    {
        switch (expansion)
        {
            case Expansion::Vanilla: return { "_classic_era_" };
            case Expansion::TBC: return { "_anniversary_" };
            case Expansion::WOTLK: return {};
            case Expansion::Forever: return { "_forever_" };
        }
        return {};
    }

    std::optional<Expansion> ExpansionFromKey(const wxString& key)
    {
        for (Expansion expansion : ALL_EXPANSIONS)
        {
            if (ExpansionKey(expansion).IsSameAs(key, false))
                return expansion;
        }
        return std::nullopt;
    }
}
