#pragma once

#include <wx/datetime.h>
#include <wx/string.h>

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Core::LogScanner
{
    struct LogPiece
    {
        size_t FileIndex = 0;
        uint64_t Start = 0;
        uint64_t End = 0;
        uint32_t Checksum = 0;
    };

    struct LogGroup
    {
        wxString Zone;
        wxString Character;
        wxDateTime Started;
        wxDateTime Ended;
        int Kills = 0;
        int Wipes = 0;
        uint64_t Bytes = 0;
        std::vector<LogPiece> Pieces;
    };

    struct LogFile
    {
        wxString Path;
        uint64_t Size = 0;
        wxDateTime Modified;
        std::string Header;
    };

    struct ScanResult
    {
        std::vector<LogFile> Files;
        std::vector<LogGroup> Groups;
        wxString Error;
    };

    ScanResult Scan(const std::vector<wxString>& paths, const std::function<void(int)>& onProgress, const std::atomic<bool>& cancelled);
}
