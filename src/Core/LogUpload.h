#pragma once

#include "Core/LogScanner.h"

#include <wx/string.h>

#include <atomic>
#include <functional>
#include <vector>

namespace Core::LogUpload
{
    struct UploadRequest
    {
        LogScanner::ScanResult Scan;
        std::vector<size_t> SelectedGroups;
        wxString Token;
        wxString Name;
        wxString WarcraftLogsID;
        long long GuildID = 0;
        long long ExpansionID = 0;
        bool IsPublic = true;
    };

    struct UploadResult
    {
        bool Ok = false;
        wxString Message;
    };

    UploadResult Run(const UploadRequest& request, const std::function<void(const wxString&, int)>& onProgress, const std::atomic<bool>& cancelled);
}
