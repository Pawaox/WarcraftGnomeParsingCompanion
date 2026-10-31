#include "Ui/FolderPromptView.h"

#include <wx/button.h>
#include <wx/dirdlg.h>
#include <wx/sizer.h>

namespace Ui
{
    namespace
    {
        constexpr int MESSAGE_WIDTH = 480;
    }

    FolderPromptView::FolderPromptView(wxWindow* parent, std::function<void(const wxString&)> onFolderChosen)
        : wxPanel(parent),
          _onFolderChosen(std::move(onFolderChosen))
    {
        _title = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
        _title->SetFont(GetFont().Bold().Scaled(1.4f));

        _message = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);

        auto* chooseButton = new wxButton(this, wxID_ANY, "Choose folder...");
        chooseButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnChooseFolder(); });

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddStretchSpacer();
        sizer->Add(_title, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(12));
        sizer->Add(_message, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, FromDIP(20));
        sizer->Add(chooseButton, 0, wxALIGN_CENTER_HORIZONTAL);
        sizer->AddStretchSpacer();
        SetSizer(sizer);
    }

    void FolderPromptView::SetPrompt(const wxString& title, const wxString& message, const wxString& startFolder)
    {
        _title->SetLabelText(title);
        _message->SetLabelText(message);
        _message->Wrap(FromDIP(MESSAGE_WIDTH));
        _startFolder = startFolder;
        Layout();
    }

    void FolderPromptView::OnChooseFolder()
    {
        wxDirDialog dialog(this, _title->GetLabelText(), _startFolder, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
        if (dialog.ShowModal() == wxID_OK)
            _onFolderChosen(dialog.GetPath());
    }
}
