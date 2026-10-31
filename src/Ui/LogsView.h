#pragma once

#include "Core/Expansion.h"

#include <wx/panel.h>
#include <wx/stattext.h>

namespace Ui
{
    class LogsView : public wxPanel
    {
    public:
        explicit LogsView(wxWindow* parent);

        void ShowExpansion(Core::Expansion expansion, const wxString& clientFolder);

    private:
        wxStaticText* _title = nullptr;
        wxStaticText* _folder = nullptr;
    };
}
