#pragma once

#include <wx/panel.h>
#include <wx/stattext.h>

#include <functional>

namespace Ui
{
    class FolderPromptView : public wxPanel
    {
    public:
        FolderPromptView(wxWindow* parent, std::function<void(const wxString&)> onFolderChosen);

        void SetPrompt(const wxString& title, const wxString& message, const wxString& startFolder);

    private:
        void OnChooseFolder();

        std::function<void(const wxString&)> _onFolderChosen;
        wxStaticText* _title = nullptr;
        wxStaticText* _message = nullptr;
        wxString _startFolder;
    };
}
