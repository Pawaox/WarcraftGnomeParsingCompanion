#include "Ui/UpdateBar.h"

#include "Ui/Palette.h"

#include <wx/sizer.h>

namespace Ui
{
    UpdateBar::UpdateBar(wxWindow* parent, std::function<void()> onUpdate)
        : wxPanel(parent)
    {
        const Palette& palette = Palette::Current();
        SetBackgroundColour(palette.NavBorder);

        _message = new wxStaticText(this, wxID_ANY, wxEmptyString);
        _message->SetForegroundColour(*wxWHITE);

        _update = new Controls::Button(this, "Update", std::move(onUpdate));

        auto* sizer = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(_message, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, FromDIP(16));
        sizer->Add(_update, 0, wxALIGN_CENTER_VERTICAL | wxALL, FromDIP(8));
        SetSizer(sizer);
    }

    void UpdateBar::ShowAvailable(const wxString& version)
    {
        SetState("Version " + version + " is ready", "Update", true);
    }

    void UpdateBar::ShowDownloading()
    {
        SetState("Downloading...", "Update", false);
    }

    void UpdateBar::ShowProblem(const wxString& message)
    {
        SetState(message, "Try again", true);
    }

    void UpdateBar::SetState(const wxString& message, const wxString& buttonLabel, bool buttonEnabled)
    {
        _message->SetLabelText(message);
        _update->SetLabelText(buttonLabel);
        _update->Enable(buttonEnabled);
        Layout();
    }
}
