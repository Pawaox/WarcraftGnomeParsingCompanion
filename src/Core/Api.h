#pragma once

#include <nlohmann/json.hpp>
#include <wx/string.h>

#include <cstddef>
#include <vector>

namespace Core::Api
{
    struct Response
    {
        bool Ok = false;
        wxString Error;
        nlohmann::json Result;
    };

    struct Guild
    {
        long long ID = 0;
        wxString Name;
        wxString ServerName;
        long long ExpansionID = 0;
        wxString ExpansionCode;
    };

    struct ExpansionInfo
    {
        long long ID = 0;
        wxString Code;
    };

    void SetBaseUrl(const wxString& baseUrl);
    wxString LoginPageUrl();

    Response Post(const wxString& path, const nlohmann::json& body, const wxString& token = wxString());
    Response PostChunk(const wxString& token, const wxString& uploadKey, int chunkIndex, const void* data, size_t size);
    std::vector<Guild> ParseGuilds(const nlohmann::json& profile);
    std::vector<ExpansionInfo> ParseExpansions(const nlohmann::json& profile);

    wxString FromJson(const nlohmann::json& value);
    std::string ToJson(const wxString& value);
}
