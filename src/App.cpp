#include "Core/Api.h"
#include "Core/AppInfo.h"
#include "Core/AppPaths.h"
#include "Core/Settings.h"
#include "Core/Startup.h"
#include "Core/Updater.h"
#include "Ui/Fonts.h"
#include "Ui/MainFrame.h"
#include "Ui/Palette.h"

#include <wx/app.h>
#include <wx/config.h>
#include <wx/image.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/snglinst.h>
#include <wx/taskbar.h>
#include <wx/utils.h>

#include <memory>

namespace
{
    constexpr int UPDATE_WAIT_ATTEMPTS = 100;
    constexpr int UPDATE_WAIT_MS = 100;

    wxApp::Appearance ToAppearance(Core::ThemeMode theme)
    {
        switch (theme)
        {
            case Core::ThemeMode::Light: return wxApp::Appearance::Light;
            case Core::ThemeMode::Dark: return wxApp::Appearance::Dark;
            case Core::ThemeMode::System: break;
        }
        return wxApp::Appearance::System;
    }
}

class App : public wxApp
{
public:
    bool OnInit() override;

private:
    bool ClaimInstance();
    bool WaitForInstance();
    bool HasArgument(const wxString& argument) const;

    std::unique_ptr<wxSingleInstanceChecker> _instanceChecker;
    Core::Settings _settings;
};

bool App::OnInit()
{
    SetAppName(Core::AppInfo::ID);
    SetAppDisplayName(Core::AppInfo::NAME);
    wxConfigBase::DontCreateOnDemand();
    wxImage::AddHandler(new wxPNGHandler());
    Ui::Fonts::Register();

    wxString exeDirectory = Core::AppPaths::ExeDirectory();
    if (!Core::AppPaths::IsDirectoryWritable(exeDirectory))
    {
        wxMessageBox(wxString(Core::AppInfo::NAME) + " keeps its settings in the folder it runs from, but it can't write to\n\n"
            + exeDirectory + "\n\nMove the app to a folder you can write to.", Core::AppInfo::NAME, wxOK | wxICON_ERROR);
        return false;
    }

    bool startedInTray = HasArgument(Core::AppInfo::TRAY_ARGUMENT);
    bool claimed = HasArgument(Core::AppInfo::UPDATED_ARGUMENT) ? WaitForInstance() : ClaimInstance();
    if (!claimed)
    {
        if (!startedInTray)
            wxMessageBox(wxString(Core::AppInfo::NAME) + " is already running. Open it from its tray icon.",
                Core::AppInfo::NAME, wxOK | wxICON_INFORMATION);
        return false;
    }

    Core::Updater::RemoveOldVersion();

    _settings = Core::Settings::Load();
    SetAppearance(ToAppearance(_settings.Theme));
    Ui::Palette::Use(_settings.Theme == Core::ThemeMode::System
        ? wxSystemSettings::GetAppearance().IsDark()
        : _settings.Theme == Core::ThemeMode::Dark);
    if (_settings.RunOnStartup)
        Core::Startup::Enable();

    Core::Api::OpenSession();

    auto* frame = new Ui::MainFrame(_settings);
    if (!startedInTray || !wxTaskBarIcon::IsAvailable())
        frame->Show();

    return true;
}

bool App::ClaimInstance()
{
    _instanceChecker = std::make_unique<wxSingleInstanceChecker>();
    _instanceChecker->Create(wxString(Core::AppInfo::ID) + "-" + wxGetUserId(), Core::AppPaths::ExeDirectory());
    return !_instanceChecker->IsAnotherRunning();
}

bool App::WaitForInstance()
{
    for (int attempt = 0; attempt < UPDATE_WAIT_ATTEMPTS; ++attempt)
    {
        if (ClaimInstance())
            return true;
        wxMilliSleep(UPDATE_WAIT_MS);
    }
    return false;
}

bool App::HasArgument(const wxString& argument) const
{
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == argument)
            return true;
    }
    return false;
}

wxIMPLEMENT_APP(App);
