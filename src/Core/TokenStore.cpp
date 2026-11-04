#include "Core/TokenStore.h"

#include "Core/AppPaths.h"

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/log.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#include <dpapi.h>
#endif

#include <string>
#include <vector>

namespace Core::TokenStore
{
    namespace
    {
        const wxString FILE_NAME = "session.dat";

        wxString TokenFile()
        {
            return wxFileName(AppPaths::ExeDirectory(), FILE_NAME).GetFullPath();
        }

#ifdef __WXMSW__
        std::vector<char> Transform(const std::vector<char>& input, bool protect)
        {
            DATA_BLOB in{ static_cast<DWORD>(input.size()), reinterpret_cast<BYTE*>(const_cast<char*>(input.data())) };
            DATA_BLOB out{};
            BOOL succeeded = protect
                ? ::CryptProtectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)
                : ::CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out);
            if (!succeeded)
                return {};

            std::vector<char> output(reinterpret_cast<char*>(out.pbData), reinterpret_cast<char*>(out.pbData) + out.cbData);
            ::LocalFree(out.pbData);
            return output;
        }
#else
        std::vector<char> Transform(const std::vector<char>& input, bool)
        {
            return input;
        }
#endif
    }

    wxString Load()
    {
        wxLogNull noLog;
        wxFFile file(TokenFile(), "rb");
        if (!file.IsOpened())
            return wxEmptyString;

        std::vector<char> data(static_cast<size_t>(file.Length()));
        if (data.empty() || file.Read(data.data(), data.size()) != data.size())
            return wxEmptyString;

        std::vector<char> plain = Transform(data, false);
        return wxString::FromUTF8(plain.data(), plain.size());
    }

    void Save(const wxString& token)
    {
        wxLogNull noLog;
        std::string utf8 = token.utf8_string();
        std::vector<char> data = Transform(std::vector<char>(utf8.begin(), utf8.end()), true);

        wxFFile file(TokenFile(), "wb");
        if (file.IsOpened() && !data.empty())
            file.Write(data.data(), data.size());
    }

    void Clear()
    {
        wxLogNull noLog;
        wxRemoveFile(TokenFile());
    }
}
