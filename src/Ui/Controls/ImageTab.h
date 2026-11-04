#pragma once

#include "Ui/Controls/Tab.h"

#include <wx/bmpbndl.h>

namespace Ui::Controls
{
    class ImageTab : public Tab
    {
    public:
        ImageTab(wxWindow* parent, const wxBitmapBundle& icon, const wxString& tooltip, std::function<void()> onClick);

    protected:
        void Paint(wxDC& dc) override;
        wxSize DoGetBestClientSize() const override;

    private:
        wxBitmapBundle _icon;
        wxBitmapBundle _dimmedIcon;
    };
}
