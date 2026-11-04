#include "Ui/UpdateRequiredView.h"

#include "Ui/Fonts.h"
#include "Ui/Images.h"
#include "Ui/Palette.h"

#include <wx/sizer.h>
#include <wx/statbmp.h>

namespace Ui
{
    namespace
    {
        constexpr int LOGO_SIZE = 96;
    }

    UpdateRequiredView::UpdateRequiredView(wxWindow* parent)
        : wxPanel(parent)
    {
        const Palette& palette = Palette::Current();
        SetBackgroundColour(palette.Background);

        auto* logo = new wxStaticBitmap(this, wxID_ANY, Images::Logo(LOGO_SIZE));

        auto* title = new wxStaticText(this, wxID_ANY, "Update required", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
        title->SetFont(Fonts::Bold(15.0));
        title->SetForegroundColour(palette.Title);

        _message = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
        _message->SetForegroundColour(palette.Text);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddStretchSpacer();
        sizer->Add(logo, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(16));
        sizer->Add(title, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(12));
        sizer->Add(_message, 0, wxALIGN_CENTER_HORIZONTAL);
        sizer->AddStretchSpacer();
        SetSizer(sizer);
    }

    void UpdateRequiredView::SetVersion(const wxString& version)
    {
        _message->SetLabelText("This version is too old. Update to " + version + " below.");
        Layout();
    }
}
