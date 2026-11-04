#include "Ui/LogPickerDialog.h"

#include "Ui/Controls/Button.h"
#include "Ui/Fonts.h"
#include "Ui/Images.h"
#include "Ui/Palette.h"

#include <wx/filename.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include <algorithm>
#include <cstdlib>

namespace Ui
{
    namespace
    {
        constexpr int MARGIN = 16;
        constexpr int WIDTH = 560;
        constexpr int HEIGHT = 480;
        constexpr int ROW_GAP = 8;

        const wxString DATE_FORMAT = "%d %b %Y  %H:%M";

        wxDateTime FileTime(const Core::CombatLogs::CombatLogFile& file)
        {
            return file.Started.IsValid() ? file.Started : file.Modified;
        }

        double SecondsApart(const Core::CombatLogs::CombatLogFile& left, const Core::CombatLogs::CombatLogFile& right)
        {
            return std::abs((FileTime(left) - FileTime(right)).GetSeconds().ToDouble());
        }
    }

    LogPickerDialog::LogPickerDialog(wxWindow* parent, const Core::CombatLogs::CombatLogFile& file, const std::vector<Core::CombatLogs::CombatLogFile>& others)
        : wxDialog(parent, wxID_ANY, "Upload", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
          _path(file.Path)
    {
        const Palette& palette = Palette::Current();
        SetBackgroundColour(palette.Background);
        SetFont(Fonts::Regular());

        auto* title = new wxStaticText(this, wxID_ANY, "Is this the only file?");
        title->SetFont(Fonts::Bold(13.0));
        title->SetForegroundColour(palette.Title);

        auto* message = new wxStaticText(this, wxID_ANY, "If the raid is split over several files, select the others too. The closest in time are at the top.");
        message->SetForegroundColour(palette.Text);
        message->Wrap(FromDIP(WIDTH - 2 * MARGIN));

        auto* list = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
        list->SetBackgroundColour(palette.Background);
        list->SetScrollRate(0, FromDIP(12));
        auto* listSizer = new wxBoxSizer(wxVERTICAL);

        std::vector<Core::CombatLogs::CombatLogFile> sorted = others;
        std::ranges::sort(sorted, [&file](const auto& left, const auto& right) { return SecondsApart(left, file) < SecondsApart(right, file); });

        for (const Core::CombatLogs::CombatLogFile& other : sorted)
        {
            auto* row = new Controls::SelectRow(list, FileTime(other).Format(DATE_FORMAT), other.Name + "   " + wxFileName::GetHumanReadableSize(other.Size, "0 B"), [] {});
            listSizer->Add(row, 0, wxEXPAND | wxBOTTOM, FromDIP(ROW_GAP));
            _choices.push_back({ other.Path, row });
        }
        list->SetSizer(listSizer);

        auto* continueButton = new Controls::Button(this, "Continue", [this] { EndModal(wxID_OK); }, Controls::Button::Style::Primary);
        auto* cancelButton = new Controls::Button(this, "Cancel", [this] { EndModal(wxID_CANCEL); });

        auto* buttons = new wxBoxSizer(wxHORIZONTAL);
        buttons->AddStretchSpacer();
        buttons->Add(cancelButton, 0, wxRIGHT, FromDIP(8));
        buttons->Add(continueButton);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(title, 0, wxALL, FromDIP(MARGIN));
        sizer->Add(message, 0, wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(MARGIN));
        sizer->Add(list, 1, wxEXPAND | wxLEFT | wxRIGHT, FromDIP(MARGIN));
        sizer->Add(buttons, 0, wxEXPAND | wxALL, FromDIP(MARGIN));
        SetSizer(sizer);

        SetSize(FromDIP(wxSize(WIDTH, HEIGHT)));
        CentreOnParent();
    }

    std::vector<wxString> LogPickerDialog::SelectedPaths() const
    {
        std::vector<wxString> paths = { _path };
        for (const Choice& choice : _choices)
        {
            if (choice.Row->IsSelected())
                paths.push_back(choice.Path);
        }
        return paths;
    }
}
