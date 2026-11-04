#pragma once

#include "Ui/Controls/Tab.h"

namespace Ui::Controls
{
    class TextTab : public Tab
    {
    public:
        TextTab(wxWindow* parent, const wxString& label, std::function<void()> onClick);

    protected:
        void Paint(wxDC& dc) override;
        wxSize DoGetBestClientSize() const override;

    private:
        wxString _label;
    };
}
