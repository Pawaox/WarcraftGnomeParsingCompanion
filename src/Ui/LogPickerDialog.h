#pragma once

#include "Core/CombatLogs.h"
#include "Ui/Controls/SelectRow.h"

#include <wx/dialog.h>

#include <vector>

namespace Ui
{
    class LogPickerDialog : public wxDialog
    {
    public:
        LogPickerDialog(wxWindow* parent, const Core::CombatLogs::CombatLogFile& file, const std::vector<Core::CombatLogs::CombatLogFile>& others);

        std::vector<wxString> SelectedPaths() const;

    private:
        struct Choice
        {
            wxString Path;
            Controls::SelectRow* Row = nullptr;
        };

        wxString _path;
        std::vector<Choice> _choices;
    };
}
