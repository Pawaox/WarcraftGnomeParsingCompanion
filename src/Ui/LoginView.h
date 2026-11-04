#pragma once

#include "Ui/Controls/Button.h"

#include <wx/panel.h>
#include <wx/stattext.h>

#include <atomic>
#include <functional>
#include <memory>

namespace Ui
{
    class LoginView : public wxPanel
    {
    public:
        LoginView(wxWindow* parent, std::function<void(const wxString&)> onLoggedIn);

        void ShowMessage(const wxString& message);

    private:
        void OnLogin();
        void OnCodeReady(const wxString& userCode);
        void OnFinished(const wxString& token, const wxString& error);
        void SetCode(const wxString& code);
        void OnCopy();

        std::function<void(const wxString&)> _onLoggedIn;
        std::shared_ptr<std::atomic<bool>> _attemptCancelled;

        wxStaticText* _message = nullptr;
        wxStaticText* _code = nullptr;
        Controls::Button* _copy = nullptr;
        Controls::Button* _login = nullptr;
    };
}
