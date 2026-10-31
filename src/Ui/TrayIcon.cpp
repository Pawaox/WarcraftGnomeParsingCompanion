#include "Ui/TrayIcon.h"

#include "Core/AppInfo.h"
#include "Ui/Images.h"

namespace Ui
{
    namespace
    {
        constexpr int ID_OPEN = wxID_HIGHEST + 1;
    }

    TrayIcon::TrayIcon(std::function<void()> onOpen, std::function<void()> onExit)
        : _onOpen(std::move(onOpen)),
          _onExit(std::move(onExit))
    {
        SetIcon(Images::Logo(16), Core::AppInfo::NAME);

        Bind(wxEVT_TASKBAR_LEFT_UP, [this](wxTaskBarIconEvent&) { _onOpen(); });
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { _onOpen(); }, ID_OPEN);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { _onExit(); }, wxID_EXIT);
    }

    wxMenu* TrayIcon::CreatePopupMenu()
    {
        auto* menu = new wxMenu();
        menu->Append(ID_OPEN, wxString("Open ") + Core::AppInfo::NAME);
        menu->AppendSeparator();
        menu->Append(wxID_EXIT, "Exit");
        return menu;
    }
}
