#pragma once

#include <wx/menu.h>
#include <wx/taskbar.h>

#include <functional>

namespace Ui
{
    class TrayIcon : public wxTaskBarIcon
    {
    public:
        TrayIcon(std::function<void()> onOpen, std::function<void()> onExit);

    protected:
        wxMenu* CreatePopupMenu() override;

    private:
        std::function<void()> _onOpen;
        std::function<void()> _onExit;
    };
}
