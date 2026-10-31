#include "Core/AppInfo.h"
#include "Core/AppPaths.h"
#include "Core/Settings.h"
#include "Core/Startup.h"
#include "Ui/MainFrame.h"

#include <wx/app.h>
#include <wx/config.h>
#include <wx/image.h>
#include <wx/msgdlg.h>
#include <wx/snglinst.h>
#include <wx/taskbar.h>
#include <wx/utils.h>

#include <memory>

namespace
{
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
    bool StartedInTray() const;

    std::unique_ptr<wxSingleInstanceChecker> _instanceChecker;
    Core::Settings _settings;
};

bool App::OnInit()
{
    SetAppName(Core::AppInfo::ID);
    SetAppDisplayName(Core::AppInfo::NAME);
    wxConfigBase::DontCreateOnDemand();
    wxImage::AddHandler(new wxPNGHandler());

    wxString exeDirectory = Core::AppPaths::ExeDirectory();
    if (!Core::AppPaths::IsDirectoryWritable(exeDirectory))
    {
        wxMessageBox(wxString(Core::AppInfo::NAME) + " keeps its settings in the folder it runs from, but it can't write to\n\n"
            + exeDirectory + "\n\nMove the app to a folder you can write to.", Core::AppInfo::NAME, wxOK | wxICON_ERROR);
        return false;
    }

    _instanceChecker = std::make_unique<wxSingleInstanceChecker>();
    _instanceChecker->Create(wxString(Core::AppInfo::ID) + "-" + wxGetUserId(), exeDirectory);
    if (_instanceChecker->IsAnotherRunning())
    {
        if (!StartedInTray())
            wxMessageBox(wxString(Core::AppInfo::NAME) + " is already running. Open it from its tray icon.",
                Core::AppInfo::NAME, wxOK | wxICON_INFORMATION);
        return false;
    }

    _settings = Core::Settings::Load();
    SetAppearance(ToAppearance(_settings.Theme));
    if (_settings.RunOnStartup)
        Core::Startup::Enable();

    auto* frame = new Ui::MainFrame(_settings);
    if (!StartedInTray() || !wxTaskBarIcon::IsAvailable())
        frame->Show();

    return true;
}

bool App::StartedInTray() const
{
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i] == Core::AppInfo::TRAY_ARGUMENT)
            return true;
    }
    return false;
}

wxIMPLEMENT_APP(App);
