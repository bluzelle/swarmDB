#ifndef KEPLER_BZNODELISTVIEW_H
#define KEPLER_BZNODELISTVIEW_H


//#include "BZDataSource.h"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/listctrl.h>
#include <thread>

class BZNodeListView : public wxListCtrl {
public:
    BZNodeListView ( wxWindow* parent, const wxPoint& pos, const wxSize& size);
    void AddItem(std::thread::id& id) {
    };



    void RefreshFromData();
    //BZDataSource* dataSource; // TODO do this later..
};


#endif //KEPLER_BZNODELISTVIEW_H
