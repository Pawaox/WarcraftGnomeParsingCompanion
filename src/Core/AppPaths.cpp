#include "Core/AppPaths.h"

#include "Core/AppInfo.h"

#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

namespace Core::AppPaths
{
    wxString ExePath()
    {
        return wxStandardPaths::Get().GetExecutablePath();
    }

    wxString ExeDirectory()
    {
        return wxFileName(ExePath()).GetPath();
    }

    wxString SettingsFile()
    {
        return wxFileName(ExeDirectory(), wxString(AppInfo::ID) + ".ini").GetFullPath();
    }

    wxString ArchiveFallbackDirectory()
    {
        wxFileName directory = wxFileName::DirName(ExeDirectory());
        directory.AppendDir("ArchivedLogs");
        return directory.GetPath();
    }

    bool IsDirectoryWritable(const wxString& directory)
    {
        wxLogNull noLog;
        wxString testFile = wxFileName(directory, ".gnomeparsing-write-test").GetFullPath();

        wxFile file;
        if (!file.Create(testFile, true))
            return false;

        file.Close();
        return wxRemoveFile(testFile);
    }
}
