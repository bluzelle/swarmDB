#ifndef KEPLER_BZAPPLICATIONLOGLISTVIEW_H
#define KEPLER_BZAPPLICATIONLOGLISTVIEW_H

#include <wx/listctrl.h>

class BZApplicationLogListView : public wxListView {
public:
    BZApplicationLogListView(
            wxWindow* parent,
            const wxPoint& pos,
            const wxSize& size
    );
    void AddLogText(const char* txt);
};

#endif //KEPLER_BZAPPLICATIONLOGLISTVIEW_H
