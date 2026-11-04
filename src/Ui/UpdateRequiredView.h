#pragma once

#include <wx/panel.h>
#include <wx/stattext.h>

namespace Ui
{
    class UpdateRequiredView : public wxPanel
    {
    public:
        explicit UpdateRequiredView(wxWindow* parent);

        void SetVersion(const wxString& version);

    private:
        wxStaticText* _message = nullptr;
    };
}
