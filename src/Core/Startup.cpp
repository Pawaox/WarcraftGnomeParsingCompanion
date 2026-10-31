#include "Core/Startup.h"

#include "Core/AppInfo.h"
#include "Core/AppPaths.h"

#include <wx/log.h>

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#else
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/utils.h>
#endif

namespace Core::Startup
{
#ifdef __WXMSW__
    namespace
    {
        const wxString RUN_KEY = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    }

    bool Enable()
    {
        wxLogNull noLog;
        wxRegKey key(wxRegKey::HKCU, RUN_KEY);
        if (!key.Create())
            return false;
        return key.SetValue(AppInfo::ID, "\"" + AppPaths::ExePath() + "\" " + AppInfo::TRAY_ARGUMENT);
    }

    bool Disable()
    {
        wxLogNull noLog;
        wxRegKey key(wxRegKey::HKCU, RUN_KEY);
        if (!key.Exists() || !key.HasValue(AppInfo::ID))
            return true;
        return key.DeleteValue(AppInfo::ID);
    }

    wxString EntryLocation()
    {
        return "the Windows startup list (HKEY_CURRENT_USER\\" + RUN_KEY + ")";
    }
#else
    namespace
    {
        bool WriteTextFile(const wxString& path, const wxString& content)
        {
            if (!wxFileName::Mkdir(wxFileName(path).GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
                return false;

            wxFile file;
            if (!file.Create(path, true))
                return false;
            return file.Write(content);
        }

        wxString EscapeXml(wxString value)
        {
            value.Replace("&", "&amp;");
            value.Replace("<", "&lt;");
            value.Replace(">", "&gt;");
            return value;
        }

#ifdef __WXOSX__
        const wxString LAUNCH_AGENT_LABEL = "dk.pawaox.gnomeparsing-companion";

        wxString EntryFile()
        {
            return wxGetHomeDir() + "/Library/LaunchAgents/" + LAUNCH_AGENT_LABEL + ".plist";
        }

        wxString EntryContent()
        {
            return "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
                "<plist version=\"1.0\">\n"
                "<dict>\n"
                "    <key>Label</key>\n"
                "    <string>" + LAUNCH_AGENT_LABEL + "</string>\n"
                "    <key>ProgramArguments</key>\n"
                "    <array>\n"
                "        <string>" + EscapeXml(AppPaths::ExePath()) + "</string>\n"
                "        <string>" + AppInfo::TRAY_ARGUMENT + "</string>\n"
                "    </array>\n"
                "    <key>RunAtLoad</key>\n"
                "    <true/>\n"
                "</dict>\n"
                "</plist>\n";
        }
#else
        wxString EntryFile()
        {
            wxString configHome;
            if (!wxGetEnv("XDG_CONFIG_HOME", &configHome) || configHome.empty())
                configHome = wxGetHomeDir() + "/.config";
            return configHome + "/autostart/gnomeparsing-companion.desktop";
        }

        wxString EntryContent()
        {
            return "[Desktop Entry]\n"
                "Type=Application\n"
                "Name=" + wxString(AppInfo::NAME) + "\n"
                "Exec=\"" + AppPaths::ExePath() + "\" " + AppInfo::TRAY_ARGUMENT + "\n"
                "X-GNOME-Autostart-enabled=true\n";
        }
#endif
    }

    bool Enable()
    {
        wxLogNull noLog;
        return WriteTextFile(EntryFile(), EntryContent());
    }

    bool Disable()
    {
        wxLogNull noLog;
        wxString path = EntryFile();
        return !wxFileExists(path) || wxRemoveFile(path);
    }

    wxString EntryLocation()
    {
        return EntryFile();
    }
#endif
}
