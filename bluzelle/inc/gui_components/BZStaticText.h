#ifndef KEPLER_BZSTATICTEXT_H
#define KEPLER_BZSTATICTEXT_H

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

class BZStaticText : public wxStaticText {
public:
    BZStaticText(wxWindow* parent, const wxString& label, const wxPoint& pos);
};

#endif //KEPLER_BZSTATICTEXT_H
