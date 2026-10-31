#include "Core/LogArchive.h"

#include "Core/AppPaths.h"
#include "Core/WowFolders.h"

#include <wx/datetime.h>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

#include <vector>

namespace Core::LogArchive
{
    namespace
    {
        const wxString LOG_PATTERN = "WoWCombatLog*.txt";
        const wxString ARCHIVE_FOLDER = "_archive";

        wxString PreferredArchiveDirectory(const wxString& clientFolder)
        {
            wxFileName directory = wxFileName::DirName(WowFolders::LogsDirectory(clientFolder));
            directory.AppendDir(ARCHIVE_FOLDER);
            return directory.GetPath();
        }

        wxString FallbackArchiveDirectory(Expansion expansion)
        {
            wxFileName directory = wxFileName::DirName(AppPaths::ArchiveFallbackDirectory());
            directory.AppendDir(ExpansionKey(expansion));
            return directory.GetPath();
        }

        bool EnsureWritableDirectory(const wxString& directory)
        {
            if (!wxDirExists(directory) && !wxFileName::Mkdir(directory, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
                return false;
            return AppPaths::IsDirectoryWritable(directory);
        }

        std::vector<wxString> FindLogs(const wxString& directory)
        {
            std::vector<wxString> files;
            if (!wxDirExists(directory))
                return files;

            wxDir dir(directory);
            if (!dir.IsOpened())
                return files;

            wxString name;
            for (bool found = dir.GetFirst(&name, LOG_PATTERN, wxDIR_FILES); found; found = dir.GetNext(&name))
                files.push_back(wxFileName(directory, name).GetFullPath());
            return files;
        }

        bool IsInUse(const wxString& path)
        {
#ifdef __WXMSW__
            HANDLE handle = ::CreateFileW(path.wc_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle == INVALID_HANDLE_VALUE)
                return ::GetLastError() == ERROR_SHARING_VIOLATION;
            ::CloseHandle(handle);
            return false;
#else
            wxDateTime modified = wxFileName(path).GetModificationTime();
            return modified.IsValid() && modified > wxDateTime::Now() - wxTimeSpan::Minutes(5);
#endif
        }

        bool MoveLogFile(const wxString& source, const wxString& target)
        {
            if (wxFileExists(target))
                return false;
            if (wxRename(source, target) == 0)
                return true;
            if (!wxCopyFile(source, target, false))
                return false;
            if (wxRemoveFile(source))
                return true;

            wxRemoveFile(target);
            return false;
        }

        int DeleteLogs(Expansion expansion, const wxString& clientFolder, const wxDateTime& olderThan)
        {
            int deleted = 0;
            for (const wxString& directory : { PreferredArchiveDirectory(clientFolder), FallbackArchiveDirectory(expansion) })
            {
                for (const wxString& path : FindLogs(directory))
                {
                    if (olderThan.IsValid())
                    {
                        wxDateTime modified = wxFileName(path).GetModificationTime();
                        if (!modified.IsValid() || modified >= olderThan)
                            continue;
                    }

                    if (wxRemoveFile(path))
                        ++deleted;
                }
            }
            return deleted;
        }
    }

    std::optional<wxString> PrepareArchiveDirectory(Expansion expansion, const wxString& clientFolder)
    {
        wxLogNull noLog;

        wxString preferred = PreferredArchiveDirectory(clientFolder);
        if (EnsureWritableDirectory(preferred))
            return preferred;

        wxString fallback = FallbackArchiveDirectory(expansion);
        if (EnsureWritableDirectory(fallback))
            return fallback;

        return std::nullopt;
    }

    std::optional<ArchiveResult> ArchiveAll(Expansion expansion, const wxString& clientFolder)
    {
        std::optional<wxString> archiveDirectory = PrepareArchiveDirectory(expansion, clientFolder);
        if (!archiveDirectory)
            return std::nullopt;

        wxLogNull noLog;
        ArchiveResult result;
        for (const wxString& path : FindLogs(WowFolders::LogsDirectory(clientFolder)))
        {
            if (IsInUse(path))
            {
                ++result.InUse;
                continue;
            }

            wxString target = wxFileName(*archiveDirectory, wxFileName(path).GetFullName()).GetFullPath();
            if (MoveLogFile(path, target))
                ++result.Moved;
            else
                ++result.Failed;
        }
        return result;
    }

    int DeleteArchived(Expansion expansion, const wxString& clientFolder)
    {
        wxLogNull noLog;
        return DeleteLogs(expansion, clientFolder, wxInvalidDateTime);
    }

    int DeleteArchivedOlderThan(Expansion expansion, const wxString& clientFolder, int days)
    {
        wxLogNull noLog;
        return DeleteLogs(expansion, clientFolder, wxDateTime::Now() - wxDateSpan::Days(days));
    }
}
