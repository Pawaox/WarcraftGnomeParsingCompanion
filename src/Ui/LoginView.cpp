#include "Ui/LoginView.h"

#include "Core/Api.h"
#include "Core/AppInfo.h"
#include "Ui/Fonts.h"
#include "Ui/Images.h"
#include "Ui/Palette.h"

#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/utils.h>

#include <chrono>
#include <thread>

namespace Ui
{
    namespace
    {
        constexpr int LOGO_SIZE = 96;
        constexpr int MESSAGE_WIDTH = 480;
        constexpr int COPY_GAP = 4;
        constexpr int POLL_SECONDS = 3;
        constexpr double CODE_SIZE = 22.0;

        const wxString WELCOME = "Please login to use the companion!";
        const wxString WAITING = "Your browser opened. Log in there if it asks, then enter this code:";
    }

    LoginView::LoginView(wxWindow* parent, std::function<void(const wxString&)> onLoggedIn)
        : wxPanel(parent),
          _onLoggedIn(std::move(onLoggedIn))
    {
        const Palette& palette = Palette::Current();
        SetBackgroundColour(palette.Background);

        auto* logo = new wxStaticBitmap(this, wxID_ANY, Images::Logo(LOGO_SIZE));

        auto* title = new wxStaticText(this, wxID_ANY, Core::AppInfo::NAME, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
        title->SetFont(Fonts::Bold(15.0));
        title->SetForegroundColour(palette.Title);

        _message = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
        _message->SetForegroundColour(palette.Text);

        _code = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
        _code->SetFont(Fonts::Bold(CODE_SIZE));
        _code->SetForegroundColour(palette.Accent);

        _copy = new Controls::Button(this, wxEmptyString, [this] { OnCopy(); }, Controls::Button::Style::Bare);
        _copy->SetIcon(Controls::Button::Icon::Copy);
        _copy->SetToolTip("Copy");

        _login = new Controls::Button(this, "Log in", [this] { OnLogin(); }, Controls::Button::Style::Primary);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddStretchSpacer();
        sizer->Add(logo, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(16));
        sizer->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(12));
        sizer->Add(_message, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(16));
        auto* codeRow = new wxBoxSizer(wxHORIZONTAL);
        codeRow->AddSpacer(_copy->GetBestSize().x + FromDIP(COPY_GAP));
        codeRow->Add(_code, 0, wxALIGN_CENTER_VERTICAL);
        codeRow->Add(_copy, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(COPY_GAP));

        sizer->Add(codeRow, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(16));
        sizer->Add(_login, 0, wxALIGN_CENTER_HORIZONTAL);
        sizer->AddStretchSpacer();
        SetSizer(sizer);

        ShowMessage(WELCOME);
        _code->Hide();
        _copy->Hide();
    }

    void LoginView::ShowMessage(const wxString& message)
    {
        _message->SetLabelText(message);
        _message->Wrap(FromDIP(MESSAGE_WIDTH));
        Layout();
    }

    void LoginView::SetCode(const wxString& code)
    {
        _code->SetLabelText(code);
        _code->Show(!code.empty());
        _copy->SetIcon(Controls::Button::Icon::Copy);
        _copy->Show(!code.empty());
        Layout();
    }

    void LoginView::OnCopy()
    {
        if (!wxTheClipboard->Open())
            return;

        wxTheClipboard->SetData(new wxTextDataObject(_code->GetLabelText()));
        wxTheClipboard->Close();

        _copy->SetIcon(Controls::Button::Icon::Check);
    }

    void LoginView::OnLogin()
    {
        if (_attemptCancelled)
            *_attemptCancelled = true;

        auto cancelled = std::make_shared<std::atomic<bool>>(false);
        _attemptCancelled = cancelled;
        _login->SetLabelText("Try again");
        ShowMessage("Starting login...");
        SetCode(wxEmptyString);

        std::thread([this, cancelled]
        {
            Core::Api::Response started = Core::Api::Post("/api/companion/login/start", nlohmann::json::object());
            if (*cancelled)
                return;

            if (!started.Ok || !started.Result.is_object())
            {
                CallAfter([this, error = started.Error] { OnFinished(wxEmptyString, error); });
                return;
            }

            std::string deviceCode = started.Result.value("deviceCode", std::string());
            wxString userCode = Core::Api::FromJson(started.Result.value("userCode", nlohmann::json()));
            CallAfter([this, userCode] { OnCodeReady(userCode); });

            while (!*cancelled)
            {
                std::this_thread::sleep_for(std::chrono::seconds(POLL_SECONDS));
                if (*cancelled)
                    return;

                Core::Api::Response poll = Core::Api::Post("/api/companion/login/poll", { { "key", deviceCode } });
                if (!poll.Ok || !poll.Result.is_object())
                    continue;

                std::string status = poll.Result.value("status", std::string());
                if (status == "APPROVED")
                {
                    wxString token = Core::Api::FromJson(poll.Result.value("sessionToken", nlohmann::json()));
                    CallAfter([this, token] { OnFinished(token, wxEmptyString); });
                    return;
                }
                if (status == "EXPIRED")
                {
                    CallAfter([this] { OnFinished(wxEmptyString, "The login timed out. Please try again."); });
                    return;
                }
            }
        }).detach();
    }

    void LoginView::OnCodeReady(const wxString& userCode)
    {
        ShowMessage(WAITING);
        SetCode(userCode);
        wxLaunchDefaultBrowser(Core::Api::LoginPageUrl());
    }

    void LoginView::OnFinished(const wxString& token, const wxString& error)
    {
        SetCode(wxEmptyString);
        _login->SetLabelText("Log in");

        if (token.empty())
        {
            ShowMessage(error);
            return;
        }

        ShowMessage(WELCOME);
        _onLoggedIn(token);
    }
}
