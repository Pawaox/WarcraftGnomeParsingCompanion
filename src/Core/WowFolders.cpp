#include "Core/WowFolders.h"

#include <wx/filename.h>

namespace Core::WowFolders
{
    namespace
    {
        wxString ChildDirectory(const wxString& parent, const wxString& child)
        {
            wxFileName directory = wxFileName::DirName(parent);
            directory.AppendDir(child);
            return directory.GetPath();
        }

        bool IsDirectChildOf(const wxString& folder, const wxString& parent)
        {
            return wxFileName::DirName(wxFileName(folder).GetPath()).SameAs(wxFileName::DirName(parent));
        }
    }

    bool LooksLikeWowRoot(const wxString& root)
    {
        for (Expansion expansion : ALL_EXPANSIONS)
        {
            for (const wxString& clientFolder : ExpansionClientFolders(expansion))
            {
                if (wxDirExists(ChildDirectory(root, clientFolder)))
                    return true;
            }
        }
        return false;
    }

    void ChangeRoot(Settings& settings, const wxString& root)
    {
        for (Expansion expansion : ALL_EXPANSIONS)
        {
            wxString& folder = settings.ExpansionFolder(expansion);
            if (!folder.empty() && !settings.WowRoot.empty() && IsDirectChildOf(folder, settings.WowRoot))
                folder.clear();
        }

        settings.WowRoot = root;

        for (Expansion expansion : ALL_EXPANSIONS)
        {
            wxString& folder = settings.ExpansionFolder(expansion);
            if (!folder.empty())
                continue;

            for (const wxString& clientFolder : ExpansionClientFolders(expansion))
            {
                wxString candidate = ChildDirectory(root, clientFolder);
                if (wxDirExists(candidate))
                {
                    folder = candidate;
                    break;
                }
            }
        }
    }

    wxString LogsDirectory(const wxString& clientFolder)
    {
        return ChildDirectory(clientFolder, "Logs");
    }
}
