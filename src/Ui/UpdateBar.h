#pragma once

#include "Ui/Controls/Button.h"

#include <wx/panel.h>
#include <wx/stattext.h>

#include <functional>

namespace Ui
{
    class UpdateBar : public wxPanel
    {
    public:
        UpdateBar(wxWindow* parent, std::function<void()> onUpdate);

        void ShowAvailable(const wxString& version);
        void ShowDownloading();
        void ShowProblem(const wxString& message);

    private:
        void SetState(const wxString& message, const wxString& buttonLabel, bool buttonEnabled);

        wxStaticText* _message = nullptr;
        Controls::Button* _update = nullptr;
    };
}
