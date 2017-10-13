
#ifndef BZROOTFRAME_H
#define BZROOTFRAME_H

#include <wx/wxprec.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <boost/lockfree/queue.hpp>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <thread>

#include "ThreadManager.h"
#include "BZVisualizationPanel.h"
#include "BZStaticText.h"
#include "BZNodeListView.h"
#include "BZApplicationLogListView.h"
#include "CQueue.h"

using namespace std;

class BZRootFrame : public wxFrame {
    // TODO: rename these to reflect what they do.
    BZNodeListView *nodeListView;

public:

    static std::mutex s_rootMutex;


    BZRootFrame();
    ThreadManager* m_threadManager;
    void addTextToLog(const char* text);
    static void pushMessage(const char* target, const char* message) {
        string item(target);
        item.append("|");
        item.append(message);
        BZRootFrame::s_messageQueue.push(item);
    }
    static uint64_t GetCurrentLoopCount() {
        return s_loopCount;
    };
    static BZRootFrame* GetInstance() {return BZRootFrame::s_instance;};

    void AddNodeListItem(const std::thread::id& id) {
        stringstream s;
        s << id;
        RemoveNodeListItem(id);
        nodeListView->InsertItem(0,s.str());
    }

    void RemoveNodeListItem(const std::thread::id& id)
    {
        stringstream ss;
        ss << id;
        string s = ss.str();
        long idx = nodeListView->FindItem(0,s);
        if(-1 != idx)
            {
            nodeListView->DeleteItem(idx);
            }
    }


protected:
DECLARE_EVENT_TABLE()

private:
    static BZRootFrame* s_instance; // TODO: OH NO THIS IS BAD!!


    void MenuInit();
    void StatusBarInit();
    void OnHello(wxCommandEvent &event);
    void OnExit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnIdle(wxIdleEvent &event);
    void OnClose(wxCloseEvent &event);
    void OnTimer(wxTimerEvent &event);

    void UpdateAppLogControl();

    wxStaticBitmap *_bluzelleLogo;
    BZStaticText *nodeListTitle;
    BZStaticText *notApplicableTitle;
    BZStaticText *logTitle;

    // TODO: rename these to reflect what they do.
    wxListView *leftTopListView;
    wxListView *leftMiddleListView;
//    wxListView *leftBottomListView;

    // TODO: need a better way of sending logs to the logListView
    BZApplicationLogListView *logListView;

    // TODO: combine vFrame and visualizationPanel
    wxFrame* vFrame;
    BZVisualizationPanel *visualizationPanel;

    wxTimer m_timerIdle;
    wxString ethereumAddress;

    static uint64_t s_loopCount;

    //TODO
    static CQueue<std::string> s_messageQueue;
};

#endif //BZROOTFRAME_H
