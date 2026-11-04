#include "Core/Api.h"

#include "Core/AppInfo.h"

#include <wx/mstream.h>
#include <wx/uri.h>
#include <wx/webrequest.h>

#include <memory>

namespace Core::Api
{
    namespace
    {
        const wxString AUTH_HEADER = "Authorization";
        const wxString UPLOAD_KEY_HEADER = "X-GP-Upload-Key";
        const wxString CHUNK_INDEX_HEADER = "X-GP-Chunk-Index";
        const wxString LOGIN_PAGE = "/companion/link";
        const wxString CHUNK_PATH = "/api/upload/chunk";

        wxString baseUrl = AppInfo::WEBSITE_URL;

        wxWebSessionSync& Session()
        {
            return wxWebSessionSync::GetDefault();
        }

        wxString ErrorText(const wxWebRequestSync::Result& result, const wxWebResponse& response)
        {
            if (!result.error.empty())
                return result.error;
            if (response.IsOk())
                return wxString::Format("The website answered with status %d.", response.GetStatus());
            return "Couldn't reach the website.";
        }
    }

    void OpenSession()
    {
        Session().CreateRequest(baseUrl);
    }

    void SetBaseUrl(const wxString& url)
    {
        baseUrl = url;
    }

    wxString LoginPageUrl()
    {
        return baseUrl + LOGIN_PAGE;
    }

    Response Post(const wxString& path, const nlohmann::json& body, const wxString& token)
    {
        Response response;

        wxWebRequestSync request = Session().CreateRequest(baseUrl + path);
        request.SetMethod("POST");
        request.SetData(wxString::FromUTF8(body.dump()), "application/json");
        if (!token.empty())
            request.SetHeader(AUTH_HEADER, "Bearer " + token);

        wxWebRequestSync::Result result = request.Execute();
        wxWebResponse webResponse = request.GetResponse();

        if (!result || !webResponse.IsOk() || webResponse.GetStatus() != 200)
        {
            response.Error = ErrorText(result, webResponse);
            return response;
        }

        nlohmann::json json = nlohmann::json::parse(webResponse.AsString().utf8_string(), nullptr, false);
        if (json.is_discarded() || !json.is_object())
        {
            response.Error = "The website sent something unreadable.";
            return response;
        }

        response.Ok = json.value("ok", false);
        response.Error = FromJson(json.value("error", nlohmann::json()));
        response.Result = json.value("result", nlohmann::json());
        return response;
    }

    Response PostChunk(const wxString& token, const wxString& uploadKey, int chunkIndex, const void* data, size_t size)
    {
        Response response;

        wxWebRequestSync request = Session().CreateRequest(baseUrl + CHUNK_PATH);
        request.SetMethod("POST");
        request.SetHeader(AUTH_HEADER, "Bearer " + token);
        request.SetHeader(UPLOAD_KEY_HEADER, uploadKey);
        request.SetHeader(CHUNK_INDEX_HEADER, wxString::Format("%d", chunkIndex));
        request.SetData(std::make_unique<wxMemoryInputStream>(data, size), "application/octet-stream", static_cast<wxFileOffset>(size));

        wxWebRequestSync::Result result = request.Execute();
        wxWebResponse webResponse = request.GetResponse();

        response.Ok = !!result && webResponse.IsOk() && webResponse.GetStatus() == 200;
        if (!response.Ok)
        {
            response.Error = webResponse.IsOk() && !webResponse.AsString().empty()
                ? webResponse.AsString()
                : ErrorText(result, webResponse);
        }
        return response;
    }

    std::vector<Guild> ParseGuilds(const nlohmann::json& profile)
    {
        std::vector<Guild> guilds;
        if (!profile.is_object() || !profile.contains("guilds"))
            return guilds;

        for (const nlohmann::json& entry : profile["guilds"])
        {
            Guild guild;
            guild.ID = entry.value("id", 0LL);
            guild.Name = FromJson(entry.value("name", nlohmann::json()));
            guild.ServerName = FromJson(entry.value("serverName", nlohmann::json()));
            guild.ExpansionID = entry.value("expansionID", 0LL);
            guild.ExpansionCode = FromJson(entry.value("expansionCode", nlohmann::json()));
            guilds.push_back(guild);
        }
        return guilds;
    }

    std::vector<ExpansionInfo> ParseExpansions(const nlohmann::json& profile)
    {
        std::vector<ExpansionInfo> expansions;
        if (!profile.is_object() || !profile.contains("expansions"))
            return expansions;

        for (const nlohmann::json& entry : profile["expansions"])
        {
            ExpansionInfo expansion;
            expansion.ID = entry.value("id", 0LL);
            expansion.Code = FromJson(entry.value("code", nlohmann::json()));
            expansions.push_back(expansion);
        }
        return expansions;
    }

    wxString FromJson(const nlohmann::json& value)
    {
        return value.is_string() ? wxString::FromUTF8(value.get<std::string>()) : wxString();
    }

    std::string ToJson(const wxString& value)
    {
        return value.utf8_string();
    }
}
