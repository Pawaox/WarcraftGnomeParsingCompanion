#include "Ui/LogsView.h"

#include "Core/WowFolders.h"

#include <wx/sizer.h>

namespace Ui
{
    LogsView::LogsView(wxWindow* parent)
        : wxPanel(parent)
    {
        _title = new wxStaticText(this, wxID_ANY, wxEmptyString);
        _title->SetFont(GetFont().Bold().Scaled(1.2f));

        _folder = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_MIDDLE);

        auto* placeholder = new wxStaticText(this, wxID_ANY, wxEmptyString);
        placeholder->SetLabelText("The Logs view isn't built yet.");

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(_title, 0, wxALL, FromDIP(16));
        sizer->Add(_folder, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(16));
        sizer->Add(placeholder, 0, wxLEFT | wxRIGHT, FromDIP(16));
        SetSizer(sizer);
    }

    void LogsView::ShowExpansion(Core::Expansion expansion, const wxString& clientFolder)
    {
        _title->SetLabelText(Core::ExpansionName(expansion) + " logs");
        _folder->SetLabelText("Logs folder: " + Core::WowFolders::LogsDirectory(clientFolder));
        Layout();
    }
}
