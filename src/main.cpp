#include "KeplerSynchronizedSetWrapper.hpp"
#include "KeplerSynchronizedMapWrapper.hpp"

#include <boost/locale.hpp>
#include <boost/range/irange.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <iostream>

#include <ctime>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/listctrl.h>

#include <thread>
#include <chrono>
#include <queue>



#define MAX_THREADS 25

#define MAIN_WINDOW_TIMER_PERIOD_MILLISECONDS 20
#define THREAD_SLEEP_TIME_MILLISECONDS 40
#define MAIN_THREAD_DEATH_PRE_DELAY_MILLISECONDS 5000

#define THREAD_RANDOM_LOG_ARBITRARY_MESSAGE_PROBABILITY_PERCENTAGE 1
#define THREAD_RANDOM_DEATH_PROBABILITY_PERCENTAGE 0.1
#define CREATE_NEW_DEFICIT_THREADS_PROBABILITY_PERCENTAGE 0.5

#define MAX_LOG_ENTRIES 300
#define GLOBAL_CONTROL_BORDER 3
#define GLOBAL_CONTROL_PROPORTION_MULTIPLIER 3



class ThreadData
    {
    public:

        ThreadData(std::shared_ptr<std::thread> ptr_thread)
            {
            m_ptr_thread = ptr_thread;     
            }

        ThreadData(const ThreadData &original)
            {
            m_ptr_thread = original.m_ptr_thread;    

            m_vectorLogMessages = original.m_vectorLogMessages;
            }

        std::shared_ptr<std::thread> m_ptr_thread;
        std::vector<std::string> m_vectorLogMessages;
    };



class KeplerApplication: public wxApp
    {
    public:

        virtual bool OnInit();

        static std::unique_lock<std::mutex> getStdOutLock();

        static unsigned int sleepRandomMilliseconds(const unsigned int uintMaximumMilliseconds);

        static KeplerSynchronizedMapWrapper<std::thread::id, ThreadData> s_threads;
        static KeplerSynchronizedSetWrapper<std::thread::id> s_threadIdsToKill;

        static std::mutex s_stdOutMutex;              

        static bool s_bool_endAllThreads;
    };



KeplerSynchronizedMapWrapper<std::thread::id, ThreadData> KeplerApplication::s_threads;
KeplerSynchronizedSetWrapper<std::thread::id> KeplerApplication::s_threadIdsToKill;

std::mutex KeplerApplication::s_stdOutMutex;

bool KeplerApplication::s_bool_endAllThreads = false;



std::unique_lock<std::mutex> KeplerApplication::getStdOutLock()
    {
    std::unique_lock<std::mutex> mutexLock(s_stdOutMutex,
                                           std::defer_lock);

    mutexLock.lock();

    return mutexLock;
    }



std::string getStringFromThreadId(const std::thread::id &idThread)
    {
    std::stringstream ss;
    ss << idThread;
    
    return ss.str();
    }



int getThreadFriendlyLargeRandomNumber()
    {
    typedef unsigned long long u64;

    u64 u64useconds;
    struct timeval tv;

    gettimeofday(&tv,
                 NULL);
    u64useconds = (1000000 * tv.tv_sec) + tv.tv_usec;

    std::srand(u64useconds); // use current time as seed for random generator

    int intRandomVariable = std::rand();  

    return intRandomVariable;
    }



unsigned int KeplerApplication::sleepRandomMilliseconds(const unsigned int uintMaximumMilliseconds)
    {
    int intRandomVariable = getThreadFriendlyLargeRandomNumber();

    unsigned int uintActualMillisecondsToSleep = intRandomVariable % uintMaximumMilliseconds;

    std::this_thread::sleep_for(std::chrono::milliseconds(uintActualMillisecondsToSleep));      

    return uintActualMillisecondsToSleep;      
    }



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



BEGIN_EVENT_TABLE(KeplerFrame, wxFrame)
   EVT_IDLE(KeplerFrame::OnIdle)
   EVT_CLOSE(KeplerFrame::OnClose)
END_EVENT_TABLE()



KeplerFrame *KeplerFrame::s_ptr_global = NULL;



enum
    {
    ID_Kepler = 1
    };



wxIMPLEMENT_APP(KeplerApplication);



bool KeplerApplication::OnInit()
    {
    KeplerFrame *frame = new KeplerFrame();
    
    frame->Show(true);
    
    return true;
    }



void threadIntroduction(const unsigned int i)
    {
    std::thread::id myThreadId = std::this_thread::get_id();

    std::stringstream stringStreamOutput;
    stringStreamOutput << "A NEW node just came up with id: " << myThreadId << std::endl;
    const std::string strOutput = stringStreamOutput.str();

    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);

    std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

    std::cout << strOutput;

    lockStdOut.unlock();        

//    KeplerFrame::s_ptr_global->addThreadToListViewNodes(myThreadId);
    }

void logArbitraryThreadLogMessage(const std::thread::id &myThreadId,
                                  const std::string &strLogMessage)
    {
    KeplerApplication::s_threads.safe_use_member(myThreadId, 
                                                 [&strLogMessage,
                                                  &myThreadId] (ThreadData &threadData) 
        {
        threadData.m_vectorLogMessages.push_back(strLogMessage);

        std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

        std::cout << "Thread with id: " << myThreadId << " now has " << threadData.m_vectorLogMessages.size() << " messages in it" << std::endl;
        });
    }

void threadLifeCycleLoop(const std::thread::id &myThreadId)
    {
    int intRandomVariable = getThreadFriendlyLargeRandomNumber();
    float floatComputedRandomValue = (intRandomVariable % 1000000) * 1.0 / 10000.0;
    const bool boolThreadRandomlyLogsArbitraryMessage = (floatComputedRandomValue <= THREAD_RANDOM_LOG_ARBITRARY_MESSAGE_PROBABILITY_PERCENTAGE);

    if (boolThreadRandomlyLogsArbitraryMessage)
        {
        logArbitraryThreadLogMessage(myThreadId,
                                     getStringFromThreadId(myThreadId));        
        }
    }

void threadLifeCycle(const unsigned int i)
    {
    bool boolThreadRandomlyShouldDie = false;

    do 
        {
        unsigned int uintActualMillisecondsToSleep = KeplerApplication::sleepRandomMilliseconds(THREAD_SLEEP_TIME_MILLISECONDS);

        std::thread::id myThreadId = std::this_thread::get_id();

        std::stringstream stringStreamOutput;

        stringStreamOutput << "Node awakens after " 
                           << uintActualMillisecondsToSleep 
                           << "ms and then goes back to sleep with id: " 
                           << myThreadId 
                           << std::endl
                           << std::flush;      

        std::string strOutput = stringStreamOutput.str();          

//    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);        



        int intRandomVariable = getThreadFriendlyLargeRandomNumber();
        float floatComputedRandomValue = (intRandomVariable % 1000000) * 1.0 / 10000.0;
        boolThreadRandomlyShouldDie = (floatComputedRandomValue <= THREAD_RANDOM_DEATH_PROBABILITY_PERCENTAGE);

        threadLifeCycleLoop(myThreadId);
        }
    while ((!KeplerApplication::s_bool_endAllThreads) && 
           (!boolThreadRandomlyShouldDie));

    std::thread::id myThreadId = std::this_thread::get_id();

    std::stringstream stringStreamOutput;

    stringStreamOutput << "Node is ending its lifecycle with id: " 
                       << myThreadId 
                       << std::endl
                       << std::flush;      

    std::string strOutput = stringStreamOutput.str();          

    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);

    // We only flag the thread to be killed manually if it naturally decided to die AND the system is not already
    // set to end all threads. If the latter is true, this thread will be killed anyways. We certainly don't
    // want it being killed twice -- that will cause a crash.

    if ((boolThreadRandomlyShouldDie) && 
        (!KeplerApplication::s_bool_endAllThreads))
        {
        KeplerApplication::s_threadIdsToKill.safe_insert(myThreadId); 
        }

    std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

    std::cout << strOutput;
    }

void threadFunction(const unsigned int i) 
    {
    threadIntroduction(i);

    threadLifeCycle(i);
    }
 


std::string getCPPVersionString()
    {
    if ( __cplusplus == 201402L ) 
        {
        return "C++14" ;
        }
 
    else if ( __cplusplus == 201103L ) 
        {
        return "C++11" ;
        }
 
    else if( __cplusplus == 19971L ) 
        {
        return "C++98" ;
        }
 
    else 
        {
        return "pre-standard C++" ;      
        }
    }



void printCPPVersion()
    {
    std::cout << getCPPVersionString() << std::endl;
    }



std::string getBuildInformationString()
    {
    const std::string strRetVal = getCPPVersionString() + " - " + __DATE__ + " - " + __TIME__;

    std::cout << strRetVal;

    return strRetVal;
    }



KeplerFrame::KeplerFrame()
            :wxFrame(NULL, 
                     wxID_ANY, 
                     "Kepler TestNet Simulator - " + getBuildInformationString(),
                     wxDefaultPosition,
                     wxDefaultSize,
//                     wxSize(1000,1000),
                     wxDEFAULT_FRAME_STYLE),
             m_timerIdle(this, 
                         wxID_ANY),
             m_uintTimerCounter(0),
             m_uintIdleCounter(0),
             m_uintListCtrlApplicationWideLogCounter(0)
    {
    // SetMinSize(wxSize(300,300));
    // SetMinSize(wxSize(800,800));

    KeplerFrame::s_ptr_global = this;

    printCPPVersion();

    std::cout << "Main Node id: " << std::this_thread::get_id() << "\n";

    m_ptr_menuFile = new wxMenu;
    m_ptr_menuFile->Append(ID_Kepler, 
                     "&Welcome...\tCtrl-H",
                     "Change this text to something appropriate");   
    m_ptr_menuFile->AppendSeparator();  
    m_ptr_menuFile->Append(wxID_EXIT);


   
    m_ptr_menuHelp = new wxMenu;
    m_ptr_menuHelp->Append(wxID_ABOUT);


    
    m_ptr_menuBar = new wxMenuBar;    
    m_ptr_menuBar->Append(m_ptr_menuFile, 
                    "&File");    
    m_ptr_menuBar->Append(m_ptr_menuHelp, 
                    "&Help");    
    SetMenuBar(m_ptr_menuBar);


    
    CreateStatusBar();


    
    SetStatusText("Welcome to Kepler!");


    
    Bind(wxEVT_MENU, 
         &KeplerFrame::OnKepler, 
         this, 
         ID_Kepler);
    Bind(wxEVT_MENU, 
         &KeplerFrame::OnAbout, 
         this, 
         wxID_ABOUT);
    Bind(wxEVT_MENU, 
         &KeplerFrame::OnExit, 
         this, 
         wxID_EXIT);



    m_ptr_boxSizerApplication = new wxBoxSizer(wxVERTICAL);



    m_ptr_boxSizerTop = new wxBoxSizer(wxHORIZONTAL);



    m_ptr_boxSizerApplication->Add(m_ptr_boxSizerTop,
                                   75 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,                
                                   wxEXPAND | 
                                   wxALL,
                                   GLOBAL_CONTROL_BORDER); 



    addStaticTextApplicationWideLogIdentifier();



    addListCtrlApplicationWideLog();



    addListViewNodes();



    m_ptr_boxSizerSelectedThread = new wxBoxSizer(wxVERTICAL);



    addStaticTextThreadIdentifier();



    addListViewNodeKeyValuesStore();


// TURN THIS BACK ON WHEN YOU NEED INBOX AND OUTBOX
//    addThreadInboxOutboxControls();



    addListViewNodeAttributes();



    addTextCtrlThreadLog();



    m_ptr_boxSizerTop->Add(m_ptr_boxSizerSelectedThread,
                           50 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                           wxEXPAND | 
                           wxALL,
                           GLOBAL_CONTROL_BORDER);



    SetSizerAndFit(m_ptr_boxSizerApplication);



    m_timerIdle.Start(MAIN_WINDOW_TIMER_PERIOD_MILLISECONDS);
  
    Connect(m_timerIdle.GetId(), 
            wxEVT_TIMER, 
            wxTimerEventHandler(KeplerFrame::OnTimer), 
            NULL, 
            this); 
    }

void KeplerFrame::addListViewNodes()
    {
    m_ptr_listViewNodes = new wxListView(this, 
                                         wxID_ANY, 
                                         wxDefaultPosition, 
                                         wxDefaultSize,
                                         wxLC_REPORT | wxLC_SINGLE_SEL);

    m_ptr_listViewNodes->AppendColumn("Node Id");

    m_ptr_listViewNodes->SetColumnWidth(0, wxLIST_AUTOSIZE);

    m_ptr_boxSizerTop->Add(m_ptr_listViewNodes,
                           50 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                           wxEXPAND | 
                           wxALL,
                           GLOBAL_CONTROL_BORDER);    
    }

void KeplerFrame::addListViewNodeKeyValuesStore()
    {
    m_ptr_listViewNodeKeyValuesStore = new wxListView(this, 
                                                      wxID_ANY, 
                                                      wxDefaultPosition, 
                                                      wxDefaultSize);

    m_ptr_listViewNodeKeyValuesStore->AppendColumn("Key");
    m_ptr_listViewNodeKeyValuesStore->AppendColumn("Value");

    // Add three items to the list

    m_ptr_listViewNodeKeyValuesStore->InsertItem(0, "id1");
    m_ptr_listViewNodeKeyValuesStore->SetItem(0, 1, "Alfa");
    m_ptr_listViewNodeKeyValuesStore->InsertItem(1, "address1");
    m_ptr_listViewNodeKeyValuesStore->SetItem(1, 1, "Sesame Street");
    m_ptr_listViewNodeKeyValuesStore->InsertItem(2, "DOB1");
    m_ptr_listViewNodeKeyValuesStore->SetItem(2, 1, "18/05/1976");

    m_ptr_listViewNodeKeyValuesStore->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_ptr_listViewNodeKeyValuesStore->SetColumnWidth(1, wxLIST_AUTOSIZE);

    m_ptr_boxSizerSelectedThread->Add(m_ptr_listViewNodeKeyValuesStore,
                                      35 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                      wxEXPAND | 
                                      wxALL,
                                      GLOBAL_CONTROL_BORDER);
    }

void KeplerFrame::addTextCtrlThreadLog()
    {
    m_ptr_textCtrlThreadLog = new wxTextCtrl(this, 
                                             -1, 
                                             "", 
                                             wxDefaultPosition, 
                                             wxDefaultSize, 
                                             wxTE_MULTILINE |
                                             wxTE_READONLY |
                                             wxHSCROLL);

    m_ptr_boxSizerSelectedThread->Add(m_ptr_textCtrlThreadLog,
                                      25 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                      wxEXPAND | 
                                      wxALL,
                                      GLOBAL_CONTROL_BORDER);
    }

// void KeplerFrame::addStaticTextListViewNodesIdentifier()
//     {
//     m_ptr_staticTextListViewNodesIdentifier = new wxStaticText(this, 
//                                                                wxID_ANY,
//                                                                "Node List",
//                                                                wxDefaultPosition,
//                                                                wxDefaultSize,
//                                                                wxALIGN_CENTRE);

//     m_ptr_boxSizerSelectedThread->Add(m_ptr_staticTextListViewNodesIdentifier,
//                                       0 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
//                                       wxEXPAND | 
//                                       wxALL,
//                                       GLOBAL_CONTROL_BORDER);
//     }

void KeplerFrame::addStaticTextApplicationWideLogIdentifier()
    {
    m_ptr_staticTextApplicationWideLogIdentifier = new wxStaticText(this, 
                                                                    wxID_ANY,
                                                                    "Application-wide Log",
                                                                    wxDefaultPosition,
                                                                    wxDefaultSize,
                                                                    wxALIGN_CENTRE);

    m_ptr_boxSizerApplication->Add(m_ptr_staticTextApplicationWideLogIdentifier,
                                   0 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                   wxEXPAND | 
                                   wxALL,
                                   GLOBAL_CONTROL_BORDER);
    }

void KeplerFrame::addStaticTextThreadIdentifier()
    {
    m_ptr_staticTextThreadIdentifier = new wxStaticText(this, 
                                                        wxID_ANY,
                                                        "N/A",
                                                        wxDefaultPosition,
                                                        wxDefaultSize,
                                                        wxALIGN_CENTRE);

    m_ptr_boxSizerSelectedThread->Add(m_ptr_staticTextThreadIdentifier,
                                      0 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                      wxEXPAND | 
                                      wxALL,
                                      GLOBAL_CONTROL_BORDER);
    }

void KeplerFrame::addListViewNodeAttributes()
    { 
    m_ptr_listViewNodeAttributes = new wxListView(this, 
                                                  wxID_ANY, 
                                                  wxDefaultPosition, 
                                                  wxDefaultSize);

    m_ptr_listViewNodeAttributes->AppendColumn("Attribute Name");
    m_ptr_listViewNodeAttributes->AppendColumn("Attribute Value");

    // Add three items to the list

    m_ptr_listViewNodeAttributes->InsertItem(0, "Name");
    m_ptr_listViewNodeAttributes->SetItem(0, 1, "Alfa");
    m_ptr_listViewNodeAttributes->InsertItem(1, "Weight");
    m_ptr_listViewNodeAttributes->SetItem(1, 1, "5");
    m_ptr_listViewNodeAttributes->InsertItem(2, "Address");
    m_ptr_listViewNodeAttributes->SetItem(2, 1, "Sesame Street");

    m_ptr_listViewNodeAttributes->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_ptr_listViewNodeAttributes->SetColumnWidth(1, wxLIST_AUTOSIZE);

    m_ptr_boxSizerSelectedThread->Add(m_ptr_listViewNodeAttributes,
                                      25 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                      wxEXPAND | 
                                      wxALL,
                                      GLOBAL_CONTROL_BORDER);
    }

void KeplerFrame::addListCtrlApplicationWideLog()
    {
    m_ptr_listCtrlApplicationWideLog = new wxListView(this, 
                                                      wxID_ANY, 
                                                      wxDefaultPosition, 
                                                      wxDefaultSize);

    m_ptr_listCtrlApplicationWideLog->AppendColumn("Timer Loop #");
    m_ptr_listCtrlApplicationWideLog->AppendColumn("Entry #");
    m_ptr_listCtrlApplicationWideLog->AppendColumn("Timestamp");
    m_ptr_listCtrlApplicationWideLog->AppendColumn("Network-Wide Log");

    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(1, wxLIST_AUTOSIZE);
    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(2, wxLIST_AUTOSIZE);
    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(3, wxLIST_AUTOSIZE);
 
    m_ptr_boxSizerApplication->Add(m_ptr_listCtrlApplicationWideLog,
                                   25 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                   wxEXPAND | 
                                   wxALL,
                                   GLOBAL_CONTROL_BORDER);
    }

void KeplerFrame::addThreadInboxOutboxControls()
    {
    m_ptr_boxSizerThreadMessages = new wxBoxSizer(wxHORIZONTAL);

    m_ptr_boxSizerSelectedThread->Add(m_ptr_boxSizerThreadMessages,
                                      15 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                      wxEXPAND | 
                                      wxALL,
                                      GLOBAL_CONTROL_BORDER);



    m_ptr_listViewNodeInbox = new wxListView(this, 
                                             wxID_ANY, 
                                             wxDefaultPosition, 
                                             wxDefaultSize);

    m_ptr_listViewNodeInbox->AppendColumn("Inbox Message");

    // Add three items to the list

    m_ptr_listViewNodeInbox->InsertItem(0, "id1");
    m_ptr_listViewNodeInbox->InsertItem(1, "address1");
    m_ptr_listViewNodeInbox->InsertItem(2, "DOB1");

    m_ptr_listViewNodeInbox->SetColumnWidth(0, wxLIST_AUTOSIZE);

    m_ptr_boxSizerThreadMessages->Add(m_ptr_listViewNodeInbox,
                                      50 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                      wxEXPAND | 
                                      wxALL,
                                      GLOBAL_CONTROL_BORDER);



    m_ptr_listViewNodeOutbox = new wxListView(this, 
                                                    wxID_ANY, 
                                                    wxDefaultPosition, 
                                                    wxDefaultSize);

    m_ptr_listViewNodeOutbox->AppendColumn("Outbox Message");

    // Add three items to the list

    m_ptr_listViewNodeOutbox->InsertItem(0, "id1");
    m_ptr_listViewNodeOutbox->InsertItem(1, "address1");
    m_ptr_listViewNodeOutbox->InsertItem(2, "DOB1");

    m_ptr_listViewNodeOutbox->SetColumnWidth(0, wxLIST_AUTOSIZE);

    m_ptr_boxSizerThreadMessages->Add(m_ptr_listViewNodeOutbox,
                                      50 * GLOBAL_CONTROL_PROPORTION_MULTIPLIER,
                                      wxEXPAND | 
                                      wxALL,
                                      GLOBAL_CONTROL_BORDER);   
    }

void KeplerFrame::OnAbout(wxCommandEvent& event)
    {
    using namespace boost::locale;
    using namespace std;
    generator gen;

    // Create system default locale

    locale loc = gen(""); 

    // Make it system global

    locale::global(loc); 

    const string str = (format("Today {1,date} at {1,time} we had run Kepler") % time(0)).str();

    wxMessageBox(str,
                 "About Kepler", 
                 wxOK | wxICON_INFORMATION);
    }

void KeplerFrame::OnKepler(wxCommandEvent& event)
    {
    wxLogMessage("Hello from Kepler!");
    }

void KeplerFrame::OnExit(wxCommandEvent& event)
    {
//    std::this_thread::sleep_for(std::chrono::milliseconds(MAIN_THREAD_DEATH_PRE_DELAY_MILLISECONDS));      

    onClose();

    Close(true);
    }

void KeplerFrame::OnClose(wxCloseEvent& event)
    {
//    std::this_thread::sleep_for(std::chrono::milliseconds(MAIN_THREAD_DEATH_PRE_DELAY_MILLISECONDS));      

    onClose();

    Destroy();
    }

void KeplerFrame::onClose()
    {
    KeplerApplication::s_bool_endAllThreads = true;

    KeplerApplication::s_threads.safe_iterate([] (const std::thread::id &threadId, 
                                                  const ThreadData &currentThreadData) 
        {
        std::stringstream stringStreamOutput;
        stringStreamOutput << "Joining Node: " << threadId << std::endl;
        const std::string strOutput = stringStreamOutput.str();

        KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);

//        std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

        std::cout << strOutput;

        if (currentThreadData.m_ptr_thread->joinable())
            {
            currentThreadData.m_ptr_thread->join();
            }
        });
    }

std::unique_lock<std::mutex> KeplerFrame::getTextCtrlApplicationWideLogQueueLock()
    {
    std::unique_lock<std::mutex> mutexLock(m_textCtrlApplicationWideLogQueueMutex,
                                           std::defer_lock);

    mutexLock.lock();

    return mutexLock;
    }

// std::unique_lock<std::mutex> KeplerFrame::getListViewNodesQueueLock()
//     {
//     std::unique_lock<std::mutex> mutexLock(m_listViewNodesQueueMutex,
//                                            std::defer_lock);

//     mutexLock.lock();

//     return mutexLock;
//     }   

// void KeplerFrame::addThreadToListViewNodes(const std::thread::id &threadId)
//     {
//     std::unique_lock<std::mutex> lockListViewNodesQueue = KeplerFrame::getListViewNodesQueueLock();   

//     m_queueListViewNodes.push(threadId);
//     }

void KeplerFrame::addTextToTextCtrlApplicationWideLogQueue(const std::string &str)
    {
    std::unique_lock<std::mutex> lockTextCtrlApplicationWideLogQueue = KeplerFrame::getTextCtrlApplicationWideLogQueueLock();   

    m_queueTextCtrlApplicationWideLog.push(str);
    }

void KeplerFrame::addTextToTextCtrlApplicationWideLogFromQueue()
    {
    unsigned int counter = 0;

    if (m_ptr_listCtrlApplicationWideLog->GetItemCount() > MAX_LOG_ENTRIES)
        {
        m_ptr_listCtrlApplicationWideLog->DeleteAllItems();
        }

    std::unique_lock<std::mutex> lockTextCtrlApplicationWideLogQueue = KeplerFrame::getTextCtrlApplicationWideLogQueueLock();   

    while (!m_queueTextCtrlApplicationWideLog.empty())
        {
        counter++;
        m_uintListCtrlApplicationWideLogCounter++;

        const std::time_t timeCurrent = std::time(nullptr);
        const std::string strCurrentTime = std::asctime(std::localtime(&timeCurrent));

        const std::string &strFrontLogEvent = m_queueTextCtrlApplicationWideLog.front();

        m_ptr_listCtrlApplicationWideLog->InsertItem(0, std::to_string(m_uintTimerCounter));
        m_ptr_listCtrlApplicationWideLog->SetItem(0, 1, std::to_string(m_uintListCtrlApplicationWideLogCounter));
        m_ptr_listCtrlApplicationWideLog->SetItem(0, 2, strCurrentTime);
        m_ptr_listCtrlApplicationWideLog->SetItem(0, 3, strFrontLogEvent);

        m_queueTextCtrlApplicationWideLog.pop();
        }

    lockTextCtrlApplicationWideLogQueue.unlock();

    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(0, wxLIST_AUTOSIZE_USEHEADER);
    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(1, wxLIST_AUTOSIZE_USEHEADER);
    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(2, wxLIST_AUTOSIZE_USEHEADER);
    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(3, wxLIST_AUTOSIZE_USEHEADER);
    }

// This event only fires if there is activity. It does not ALWAYS fire.

void KeplerFrame::OnIdle(wxIdleEvent& event)
    {
    m_uintIdleCounter++;
    }

void KeplerFrame::killAndJoinThreadsIfNeeded()
    {
    std::vector<std::thread::id> vectorNewlyKilledThreadIds;

    // Access the threads object inside a critical section

    KeplerApplication::s_threads.safe_use([&vectorNewlyKilledThreadIds] (KeplerSynchronizedMapWrapper<std::thread::id, ThreadData>::KeplerSynchronizedMapType &mapThreads) 
        {
        KeplerApplication::s_threadIdsToKill.safe_iterate([&mapThreads,
                                                           &vectorNewlyKilledThreadIds] (const std::thread::id &threadId) 
            {
            std::stringstream stringStreamOutput1;
            stringStreamOutput1 << "Node with id: " << threadId << " has 'decided' to 'crash'... joining it..." << std::endl;
            const std::string strOutput1 = stringStreamOutput1.str();

            KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput1);

            std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();
            std::cout << strOutput1;
            lockStdOut.unlock();



            if (mapThreads.at(threadId).m_ptr_thread->joinable())
                {
                mapThreads.at(threadId).m_ptr_thread->join();
                }



            std::stringstream stringStreamOutput2;
            stringStreamOutput2 << "Node with id: " << threadId << " joining DONE" << std::endl;
            const std::string strOutput2 = stringStreamOutput2.str();

            KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput2);

            lockStdOut.lock();
            std::cout << strOutput2;
            lockStdOut.unlock();

            mapThreads.erase(threadId);

            vectorNewlyKilledThreadIds.push_back(threadId);
            });
        });  

    // WATCHOUT -- there could be a race condition newly deleted threads get into the s_threadIdsToKill map before the code below fires, which means those
    // newly deleted threads don't get processed as above. Maybe. Not sure. The deleting of only items in the vector probably ensures this cannot happen.

    KeplerApplication::s_threadIdsToKill.safe_use([&vectorNewlyKilledThreadIds] (KeplerSynchronizedSetWrapper<std::thread::id>::KeplerSynchronizedSetType &setThreadsToKill)
        {
        for (const auto &currentNewlyKilledThreadId : vectorNewlyKilledThreadIds)
            {
            setThreadsToKill.erase(currentNewlyKilledThreadId);

//            onNewlyCreatedThread(currentNewlyCreatedThreadId);
            }
        }); 

    for (const auto &currentNewlyKilledThreadId : vectorNewlyKilledThreadIds)
        {
        onNewlyKilledThread(currentNewlyKilledThreadId);
        }
    }

void KeplerFrame::createNewThreadsIfNeeded()
    {
    if (KeplerApplication::s_bool_endAllThreads == false)
        {
        std::vector<std::thread::id> vectorNewlyCreatedThreadIds;

        // Access the threads object inside a critical section

        KeplerApplication::s_threads.safe_use([&vectorNewlyCreatedThreadIds] (KeplerSynchronizedMapWrapper<std::thread::id, ThreadData>::KeplerSynchronizedMapType &mapThreads) 
            {
            const int intThreadCount = mapThreads.size();
                
            if (intThreadCount < MAX_THREADS)
                {
                std::stringstream stringStreamOutput1;
                stringStreamOutput1 << "Number of nodes in map: " << intThreadCount << std::endl;
                const std::string strOutput1 = stringStreamOutput1.str();

                KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput1);

                std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();
                std::cout << strOutput1;
                lockStdOut.unlock();


                // We don't actually create all the deficit threads, necessarily. We choose the number to create randomly, from 1 to the actual total number needed

                const int uintMaximumNewThreadsToCreate = (MAX_THREADS - intThreadCount);

                int intRandomVariable = getThreadFriendlyLargeRandomNumber();
                const int uintNumberOfNewThreadsToActuallyCreate = ((intRandomVariable % uintMaximumNewThreadsToCreate) + 1);



                for (const unsigned int i : boost::irange(0,
                                                          uintNumberOfNewThreadsToActuallyCreate)) 
                    {
                    std::shared_ptr<std::thread> ptr_newThread(new std::thread(threadFunction, i));

                    // Is this safe? Do we now have two ref-counted objects pointing to the thread but each with a count of 1?

                    mapThreads.insert(KeplerSynchronizedMapWrapper<std::thread::id, std::shared_ptr<std::thread>>::KeplerSynchronizedMapType::value_type(ptr_newThread->get_id(),
                                                                                                                                                         ptr_newThread));

                    vectorNewlyCreatedThreadIds.push_back(ptr_newThread->get_id());
                    }

                std::stringstream stringStreamOutput2;
                stringStreamOutput2 << "Number of nodes in map is NOW: " << mapThreads.size() << std::endl;
                const std::string strOutput2 = stringStreamOutput2.str();

                KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput2);

                lockStdOut.lock();
                std::cout << strOutput2;
                lockStdOut.unlock();
                }
            });

        for (const auto &currentNewlyCreatedThreadId : vectorNewlyCreatedThreadIds)
            {
            onNewlyCreatedThread(currentNewlyCreatedThreadId);
            }
        }
    }

void KeplerFrame::removeThreadIdFromListViewNodes(const std::thread::id &idThreadToRemove)
    {
    const long longIndexOfThreadToRemove = m_ptr_listViewNodes->FindItem(-1,
                                                                         getStringFromThreadId(idThreadToRemove));

    if (longIndexOfThreadToRemove != wxNOT_FOUND)
        {
        m_ptr_listViewNodes->DeleteItem(longIndexOfThreadToRemove);
        }
    }

void KeplerFrame::onNewlyCreatedThread(const std::thread::id &idNewlyCreatedThread)
    {
    removeThreadIdFromListViewNodes(idNewlyCreatedThread);

    m_ptr_listViewNodes->InsertItem(0, 
                                    getStringFromThreadId(idNewlyCreatedThread));

    m_ptr_listViewNodes->SetColumnWidth(0, 
                                        wxLIST_AUTOSIZE);
    }

void KeplerFrame::onNewlyKilledThread(const std::thread::id &idNewlyKilledThread)
    {
    removeThreadIdFromListViewNodes(idNewlyKilledThread);
    }

void KeplerFrame::OnTimer(wxTimerEvent &e)
    {
    m_uintTimerCounter++;

    killAndJoinThreadsIfNeeded();

    // If there are no threads in play, immediately create them (ie: skip the probability thing where we only create threads with some probability)

    bool boolNoThreadsInPlay = false;

    KeplerApplication::s_threads.safe_use([&boolNoThreadsInPlay] (KeplerSynchronizedMapWrapper<std::thread::id, ThreadData>::KeplerSynchronizedMapType &mapThreads)
        {
        if (mapThreads.empty())
            {
            boolNoThreadsInPlay = true;            
            }
        });

    if (boolNoThreadsInPlay)
        {
        createNewThreadsIfNeeded();            
        }
    
    else
        {
        int intRandomVariable = getThreadFriendlyLargeRandomNumber();
        float floatComputedRandomValue = (intRandomVariable % 1000000) * 1.0 / 10000.0;
        const bool boolCreateNewDeficitThreads = (floatComputedRandomValue <= CREATE_NEW_DEFICIT_THREADS_PROBABILITY_PERCENTAGE);

        if (boolCreateNewDeficitThreads)
            {
            createNewThreadsIfNeeded();
            }
        }    

    addTextToTextCtrlApplicationWideLogFromQueue();
    }