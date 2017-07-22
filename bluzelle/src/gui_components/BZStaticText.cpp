#include "BZStaticText.h"

BZStaticText::BZStaticText(
        wxWindow* parent,
        const wxString& label,
        const wxPoint& pos
)
        : wxStaticText(
        parent,
        wxID_ANY,
        label,
        pos,
        wxDefaultSize,
        0
)
{
    wxFont fnt = GetFont();
    fnt.SetPointSize(14);
    fnt.SetWeight(wxFONTWEIGHT_BOLD);
    SetFont(fnt);
    SetForegroundColour(wxColour(196, 196, 196));
}