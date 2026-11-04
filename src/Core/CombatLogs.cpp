#include "Core/CombatLogs.h"

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>

namespace Core::CombatLogs
{
    namespace
    {
        const wxString NAME_PREFIX = "WoWCombatLog";
        const std::string HEADER_EVENT = "COMBAT_LOG_VERSION";
        constexpr size_t HEADER_BYTES = 512;
        constexpr int MINUTES_UNTIL_IDLE = 5;

        std::optional<std::string> ReadFirstLine(const wxString& path)
        {
            wxFile file;
            if (!file.Open(path, wxFile::read))
                return std::nullopt;

            std::string line(HEADER_BYTES, '\0');
            ssize_t read = file.Read(line.data(), HEADER_BYTES);
            if (read == wxInvalidOffset)
                return std::nullopt;

            line.resize(static_cast<size_t>(read));
            line.resize(std::min(line.find_first_of("\r\n"), line.size()));
            return line;
        }

        const wxDateTime& SortTime(const CombatLogFile& file)
        {
            return file.Started.IsValid() ? file.Started : file.Modified;
        }
    }

    wxDateTime ParseTimestamp(const std::string& text, const wxDateTime& fallback)
    {
        int month = 0, day = 0, year = 0, hour = 0, minute = 0, second = 0;
        if (std::sscanf(text.c_str(), "%d/%d/%d %d:%d:%d", &month, &day, &year, &hour, &minute, &second) != 6)
        {
            if (std::sscanf(text.c_str(), "%d/%d %d:%d:%d", &month, &day, &hour, &minute, &second) != 5)
                return wxInvalidDateTime;
            year = fallback.GetYear();
        }

        if (month < 1 || month > 12 || day < 1 || day > 31)
            return wxInvalidDateTime;

        return wxDateTime(static_cast<wxDateTime::wxDateTime_t>(day), static_cast<wxDateTime::Month>(month - 1), year,
            static_cast<wxDateTime::wxDateTime_t>(hour), static_cast<wxDateTime::wxDateTime_t>(minute),
            static_cast<wxDateTime::wxDateTime_t>(second));
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
        return modified.IsValid() && modified > wxDateTime::Now() - wxTimeSpan::Minutes(MINUTES_UNTIL_IDLE);
#endif
    }

    std::vector<CombatLogFile> Find(const wxString& logsDirectory)
    {
        std::vector<CombatLogFile> files;
        if (!wxDirExists(logsDirectory))
            return files;

        wxDir dir(logsDirectory);
        if (!dir.IsOpened())
            return files;

        wxLogNull noLog;
        wxString name;
        for (bool found = dir.GetFirst(&name, "*.txt", wxDIR_FILES); found; found = dir.GetNext(&name))
        {
            CombatLogFile file;
            wxFileName fileName(logsDirectory, name);
            file.Path = fileName.GetFullPath();
            file.Name = name;
            file.Size = fileName.GetSize();
            file.Modified = fileName.GetModificationTime();

            std::optional<std::string> header = ReadFirstLine(file.Path);
            bool hasHeader = header && header->find(HEADER_EVENT) != std::string::npos;
            bool isUnwrittenLog = name.StartsWith(NAME_PREFIX) && (!header || header->empty());
            if (!hasHeader && !isUnwrittenLog)
                continue;

            if (hasHeader)
                file.Started = ParseTimestamp(*header, file.Modified);
            file.InUse = IsInUse(file.Path);
            files.push_back(file);
        }

        std::ranges::sort(files, [](const CombatLogFile& left, const CombatLogFile& right)
        {
            return SortTime(left) > SortTime(right);
        });
        return files;
    }
}
