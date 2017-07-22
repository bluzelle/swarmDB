#include "BZNodeListView.h"

#include <string>

BZNodeListView::BZNodeListView (
        wxWindow* parent,
        const wxPoint& pos,
        const wxSize& size

) : wxListCtrl(
        parent,
        wxID_ANY,
        pos,
        size,
        wxLC_REPORT,
        wxDefaultValidator,
        "nodelist"
){
    wxListItem item;
    item.SetText(_("Node Id"));
    item.SetWidth(size.GetWidth());
    InsertColumn(0, item);
}

void BZNodeListView::RefreshFromData() {
//    int index = 0;
//    for(std::string row : dataSource->GetData())
//        {
//        InsertItem(index, row.c_str());
//        }
    Update();
}

