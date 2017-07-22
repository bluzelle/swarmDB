#ifndef KEPLER_FRAME_H
#define KEPLER_FRAME_H



#include "ThreadData.hpp"

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/listctrl.h>

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>



class KeplerFrame: public wxFrame
    {
    public:
    
        KeplerFrame();

        std::unique_lock<std::mutex> getTextCtrlApplicationWideLogQueueLock();
//        std::unique_lock<std::mutex> getListViewNodesQueueLock();

        void addTextToTextCtrlApplicationWideLogQueue(const std::string &str);
//        void addThreadToListViewNodes(const std::thread::id &threadId);

        static KeplerFrame *s_ptr_global;

    protected:

        DECLARE_EVENT_TABLE()

    private:
    
        void addTextToTextCtrlApplicationWideLogFromQueue();

        void createNewThreadsIfNeeded();
        void killAndJoinThreadsIfNeeded();

        void addThreadInboxOutboxControls();
        void addStaticTextNodeListIdentifier();
        void addStaticTextApplicationWideLogIdentifier();
        void addListCtrlApplicationWideLog();
        void addListViewNodes();
        void addListViewNodeAttributes();
        void addTextCtrlThreadLog();
        void addStaticTextThreadIdentifier();
        void addListViewNodeKeyValuesStore();

        void OnKepler(wxCommandEvent& event);
        void OnExit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        void OnTimer(wxTimerEvent& event);
        void OnIdle(wxIdleEvent& event);
        void OnClose(wxCloseEvent& event);

        void onClose();
        void onNewlyCreatedThread(const std::thread::id &idNewlyCreatedThread);
        void onNewlyKilledThread(const std::thread::id &idNewlyKilledThread);

        void removeThreadIdFromListViewNodes(const std::thread::id &idThreadToRemove);
 
        wxTimer m_timerIdle;

        wxMenu *m_ptr_menuFile;
        wxMenu *m_ptr_menuHelp;

        wxMenuBar *m_ptr_menuBar;

        wxBoxSizer *m_ptr_boxSizerApplication;
        wxBoxSizer *m_ptr_boxSizerNodeList;
        wxBoxSizer *m_ptr_boxSizerTop;
        wxBoxSizer *m_ptr_boxSizerSelectedThread;
        wxBoxSizer *m_ptr_boxSizerThreadMessages;

        wxTextCtrl *m_ptr_textCtrlThreadLog;        

        wxListView *m_ptr_listCtrlApplicationWideLog;

        wxListView *m_ptr_listViewNodes;
        wxListView *m_ptr_listViewNodeKeyValuesStore;
        wxListView *m_ptr_listViewNodeInbox;
        wxListView *m_ptr_listViewNodeOutbox; 
        wxListView *m_ptr_listViewNodeAttributes;

        wxStaticText *m_ptr_staticTextNodeListIdentifier;
        wxStaticText *m_ptr_staticTextApplicationWideLogIdentifier;
        wxStaticText *m_ptr_staticTextThreadIdentifier; 

        std::mutex m_textCtrlApplicationWideLogQueueMutex;
 //       std::mutex m_listViewNodesQueueMutex;

        std::queue<std::string> m_queueTextCtrlApplicationWideLog;
//        std::queue<std::thread::id> m_queueListViewNodes;

        unsigned int m_uintTimerCounter;
        unsigned int m_uintIdleCounter;
        unsigned int m_uintListCtrlApplicationWideLogCounter;
    };



#endif // KEPLER_FRAME_H