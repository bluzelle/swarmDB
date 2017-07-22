#include "BZApplicationLogListView.h"
#include "BZRootFrame.h"

#include <ctime>
#include <vector>
#include <string>
#include <ctime>
#include <chrono>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


#define MAX_LOG_ENTRIES 300

BZApplicationLogListView::BZApplicationLogListView (
        wxWindow* parent,
        const wxPoint& pos,
        const wxSize& size
) : wxListView(parent, wxID_ANY,pos, size)
{
    AppendColumn("Timer Loop #");
    AppendColumn("Entry #");
    AppendColumn("Timestamp");
    AppendColumn("Network-Wide Log");

    SetColumnWidth( 0, 80);// count
    SetColumnWidth( 1, 50);// entry #
    SetColumnWidth( 2, 80);// timestamp
    SetColumnWidth( 3, 300);// msg
}

void BZApplicationLogListView::AddLogText(const char* txt)
{
    static uint64_t  logCount;

    string loopCount =  boost::lexical_cast<std::string>( BZRootFrame::GetCurrentLoopCount());

    InsertItem( 0, loopCount);     // timer loop #
    SetItem( 0, 1, boost::lexical_cast<std::string>(logCount));   // entry number

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now - std::chrono::hours(24));

    std::stringstream ssTp;
    ssTp << std::put_time(std::localtime(&now_c), "%F %T");

    SetItem( 0, 2, ssTp.str()); // timestamp



    SetItem( 0, 3, txt);
    if (GetItemCount() > MAX_LOG_ENTRIES)
        {
        DeleteItem(MAX_LOG_ENTRIES);
        }
    logCount++;
}
