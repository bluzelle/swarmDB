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



#define MAX_THREADS 1
#define MAIN_WINDOW_TIMER_PERIOD_MILLISECONDS 125
#define THREAD_SLEEP_TIME_MILLISECONDS 50
#define MAX_LOG_ENTRIES 60
#define THREAD_RANDOM_DEATH_PROBABILITY_PERCENTAGE 0.5
#define MAIN_THREAD_DEATH_PRE_DELAY_MILLISECONDS 5000



class KeplerApplication: public wxApp
    {
    public:

        virtual bool OnInit();

        static std::unique_lock<std::mutex> getStdOutLock();

        static unsigned int sleepRandomMilliseconds(const unsigned int uintMaximumMilliseconds);

        static KeplerSynchronizedMapWrapper<std::thread::id, std::shared_ptr<std::thread>> s_threads;
        static KeplerSynchronizedSetWrapper<std::thread::id> s_threadIdsToKill;

        static std::mutex s_stdOutMutex;              

        static bool s_bool_endAllThreads;
    };



KeplerSynchronizedMapWrapper<std::thread::id, std::shared_ptr<std::thread>> KeplerApplication::s_threads;
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

        void addTextToTextCtrlApplicationWideLogQueue(const std::string &str);

        static KeplerFrame *s_ptr_global;

    protected:

        DECLARE_EVENT_TABLE()

    private:
    
        void addTextToTextCtrlApplicationWideLogFromQueue();

        void createNewThreadsIfNeeded();
        void killAndJoinThreadsIfNeeded();

        void OnKepler(wxCommandEvent& event);
        void OnExit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        void OnTimer(wxTimerEvent& event);
        void OnIdle(wxIdleEvent& event);
        void OnClose(wxCloseEvent& event);

        void onClose();

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

        wxStaticText *m_ptr_staticTextThreadIdentifier; 

        std::mutex m_textCtrlApplicationWideLogMutex;

        std::queue<std::string> m_queueTextCtrlApplicationWideLog;

        unsigned int m_uintTimerCounter;
        unsigned int m_uintIdleCounter;
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
    stringStreamOutput << "Hello. This is NEW thread #: " << i << " that was JUST BORN with id: " << myThreadId << std::endl;
    const std::string strOutput = stringStreamOutput.str();

    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);

    std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

    std::cout << strOutput;

    lockStdOut.unlock();        
    }

void threadLifeCycleLoop(const unsigned int i)
    {
    unsigned int uintActualMillisecondsToSleep = KeplerApplication::sleepRandomMilliseconds(THREAD_SLEEP_TIME_MILLISECONDS);

    std::thread::id myThreadId = std::this_thread::get_id();

    std::stringstream stringStreamOutput;

    stringStreamOutput << "Thread #: " 
              << i 
              << " awakens after " 
              << uintActualMillisecondsToSleep 
              << "ms and then goes back to sleep with id: " 
              << myThreadId 
              << std::endl
              << std::flush;      

    std::string strOutput = stringStreamOutput.str();          

//    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);        
    }

void threadLifeCycle(const unsigned int i)
    {
    bool boolThreadRandomlyShouldDie = false;

    do 
        {
        int intRandomVariable = getThreadFriendlyLargeRandomNumber();
        float floatComputedRandomValue = (intRandomVariable % 10000) * 1.0 / 100.0;
        boolThreadRandomlyShouldDie = (floatComputedRandomValue <= THREAD_RANDOM_DEATH_PROBABILITY_PERCENTAGE);

        threadLifeCycleLoop(i);
        }
    while ((!KeplerApplication::s_bool_endAllThreads) && 
           (!boolThreadRandomlyShouldDie));

    std::thread::id myThreadId = std::this_thread::get_id();

    std::stringstream stringStreamOutput;

    stringStreamOutput << "Thread #: " 
              << i 
              << " is ending its lifecycle with id: " 
              << myThreadId 
              << std::endl
              << std::flush;      

    std::string strOutput = stringStreamOutput.str();          

    KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);

    // We only flag the thread to be killed manually if it naturally decided to die AND the system is not already
    // set to end all threads. If the latter is true, this thread will be killed anyways. We certainly don't
    // want it dying twice.

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
 
void printCPPVersion()
    {
    if ( __cplusplus == 201103L ) std::cout << "C++11\n" ;
 
    else if( __cplusplus == 19971L ) std::cout << "C++98\n" ;
 
    else std::cout << "pre-standard C++\n" ;        
    }



KeplerFrame::KeplerFrame()
            :wxFrame(NULL, 
                     wxID_ANY, 
                     "Kepler TestNet Simulator"),
             m_timerIdle(this, 
                         wxID_ANY),
             m_uintTimerCounter(0),
             m_uintIdleCounter(0)
    {
    KeplerFrame::s_ptr_global = this;

    printCPPVersion();

    std::cout << "Main thread id: " << std::this_thread::get_id() << "\n";

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
                             3,                
                             wxEXPAND | 
                             wxALL ); 



    m_ptr_listCtrlApplicationWideLog = new wxListView(this, 
                                               wxID_ANY, 
                                               wxDefaultPosition, 
                                               wxSize(150, 150));

    m_ptr_listCtrlApplicationWideLog->AppendColumn("#");
    m_ptr_listCtrlApplicationWideLog->AppendColumn("Network-Wide Log");

    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(1, wxLIST_AUTOSIZE);
 
    m_ptr_boxSizerApplication->Add(m_ptr_listCtrlApplicationWideLog,
                              2,
                              wxEXPAND | 
                              wxALL,       
                              10);



    m_ptr_listViewNodes = new wxListView(this, 
                                               wxID_ANY, 
                                               wxDefaultPosition, 
                                               wxSize(150, 150));

    m_ptr_listViewNodes->AppendColumn("Node Cardinality #");
    m_ptr_listViewNodes->AppendColumn("Node Physical Hash Id");

    // Add three items to the list

    m_ptr_listViewNodes->InsertItem(0, "1");
    m_ptr_listViewNodes->SetItem(0, 1, "Alfa");
    m_ptr_listViewNodes->InsertItem(1, "2");
    m_ptr_listViewNodes->SetItem(1, 1, "Bravo");
    m_ptr_listViewNodes->InsertItem(2, "3");
    m_ptr_listViewNodes->SetItem(2, 1, "Charlie");

    m_ptr_listViewNodes->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_ptr_listViewNodes->SetColumnWidth(1, wxLIST_AUTOSIZE);

    m_ptr_boxSizerTop->Add(m_ptr_listViewNodes,
                     1,
                     wxEXPAND | 
                     wxALL, 
                     10);



    m_ptr_boxSizerSelectedThread = new wxBoxSizer(wxVERTICAL);



    m_ptr_staticTextThreadIdentifier = new wxStaticText(this, 
                                                                wxID_ANY,
                                                                "",
                                                                wxDefaultPosition,
                                                                wxDefaultSize,
                                                                wxALIGN_CENTRE);

    m_ptr_boxSizerSelectedThread->Add(m_ptr_staticTextThreadIdentifier,
                                0,
                                wxEXPAND | 
                                wxALL,
                                10);



    m_ptr_listViewNodeKeyValuesStore = new wxListView(this, 
                                                            wxID_ANY, 
                                                            wxDefaultPosition, 
                                                            wxSize(150, 100));

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
                                2,
                                wxEXPAND | 
                                wxALL,
                                10);



    m_ptr_boxSizerThreadMessages = new wxBoxSizer(wxHORIZONTAL);

    m_ptr_boxSizerSelectedThread->Add(m_ptr_boxSizerThreadMessages,
                                0.5,
                                wxEXPAND | 
                                wxALL );



    m_ptr_listViewNodeInbox = new wxListView(this, 
                                                   wxID_ANY, 
                                                   wxDefaultPosition, 
                                                   wxSize(150, 100));

    m_ptr_listViewNodeInbox->AppendColumn("Inbox Message");

    // Add three items to the list

    m_ptr_listViewNodeInbox->InsertItem(0, "id1");
    m_ptr_listViewNodeInbox->InsertItem(1, "address1");
    m_ptr_listViewNodeInbox->InsertItem(2, "DOB1");

    m_ptr_listViewNodeInbox->SetColumnWidth(0, wxLIST_AUTOSIZE);

    m_ptr_boxSizerThreadMessages->Add(m_ptr_listViewNodeInbox,
                                1,
                                wxEXPAND | 
                                wxALL,
                                10);



    m_ptr_listViewNodeOutbox = new wxListView(this, 
                                                    wxID_ANY, 
                                                    wxDefaultPosition, 
                                                    wxSize(150, 100));

    m_ptr_listViewNodeOutbox->AppendColumn("Outbox Message");

    // Add three items to the list

    m_ptr_listViewNodeOutbox->InsertItem(0, "id1");
    m_ptr_listViewNodeOutbox->InsertItem(1, "address1");
    m_ptr_listViewNodeOutbox->InsertItem(2, "DOB1");

    m_ptr_listViewNodeOutbox->SetColumnWidth(0, wxLIST_AUTOSIZE);

    m_ptr_boxSizerThreadMessages->Add(m_ptr_listViewNodeOutbox,
                                1,
                                wxEXPAND | 
                                wxALL,
                                10);



    m_ptr_listViewNodeAttributes = new wxListView(this, 
                                                        wxID_ANY, 
                                                        wxDefaultPosition, 
                                                        wxSize(150, 100));

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
                                0.25,
                                wxEXPAND | 
                                wxALL,
                                10);



    m_ptr_textCtrlThreadLog = new wxTextCtrl(this, 
                                                   -1, 
                                                   "", 
                                                   wxDefaultPosition, 
                                                   wxSize(50,60), 
                                                   wxTE_MULTILINE |
                                                   wxTE_READONLY |
                                                   wxHSCROLL);

    m_ptr_boxSizerSelectedThread->Add(m_ptr_textCtrlThreadLog,
                                0.1,
                                wxEXPAND | 
                                wxALL,
                                10);



    m_ptr_boxSizerTop->Add(m_ptr_boxSizerSelectedThread,
                     1,
                     wxEXPAND | 
                     wxALL );



    SetSizerAndFit(m_ptr_boxSizerApplication);



    m_timerIdle.Start(MAIN_WINDOW_TIMER_PERIOD_MILLISECONDS);
  
    Connect(m_timerIdle.GetId(), 
            wxEVT_TIMER, 
            wxTimerEventHandler(KeplerFrame::OnTimer), 
            NULL, 
            this); 
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
                                                  const std::shared_ptr<std::thread> &ptr_thread) 
        {
        std::stringstream stringStreamOutput;
        stringStreamOutput << "Joining thread: " << ptr_thread->get_id() << std::endl;
        const std::string strOutput = stringStreamOutput.str();

        KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput);

//        std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

        std::cout << strOutput;

        ptr_thread->join();
        });
    }

std::unique_lock<std::mutex> KeplerFrame::getTextCtrlApplicationWideLogQueueLock()
    {
    std::unique_lock<std::mutex> mutexLock(m_textCtrlApplicationWideLogMutex,
                                           std::defer_lock);

    mutexLock.lock();

    return mutexLock;
    }

void KeplerFrame::addTextToTextCtrlApplicationWideLogQueue(const std::string &str)
    {
    std::unique_lock<std::mutex> lockTextCtrlApplicationWideLog = KeplerFrame::getTextCtrlApplicationWideLogQueueLock();   

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

        const std::string &strFrontLogEvent = m_queueTextCtrlApplicationWideLog.front();

        m_ptr_listCtrlApplicationWideLog->InsertItem(0, std::to_string(m_uintTimerCounter));
        m_ptr_listCtrlApplicationWideLog->SetItem(0, 1, strFrontLogEvent);

        m_queueTextCtrlApplicationWideLog.pop();
        }

    lockTextCtrlApplicationWideLogQueue.unlock();

    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_ptr_listCtrlApplicationWideLog->SetColumnWidth(1, wxLIST_AUTOSIZE);
    }

// This event only fires if there is activity. It does not ALWAYS fire.

void KeplerFrame::OnIdle(wxIdleEvent& event)
    {
    m_uintIdleCounter++;
    }

void KeplerFrame::killAndJoinThreadsIfNeeded()
    {
    // Access the threads object inside a critical section

    KeplerApplication::s_threads.safe_use([] (KeplerSynchronizedMapWrapper<std::thread::id, std::shared_ptr<std::thread>>::KeplerSynchronizedMapType &mapThreads) 
        {
        KeplerApplication::s_threadIdsToKill.safe_iterate([&mapThreads] (const std::thread::id &threadId) 
            {
            std::stringstream stringStreamOutput1;
            stringStreamOutput1 << threadId << " has been flagged to die... joining it..." << std::endl;
            const std::string strOutput1 = stringStreamOutput1.str();

            KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput1);

            std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();
            std::cout << strOutput1;
            lockStdOut.unlock();



            mapThreads[threadId]->join();



            std::stringstream stringStreamOutput2;
            stringStreamOutput2 << threadId << " joining DONE" << std::endl;
            const std::string strOutput2 = stringStreamOutput2.str();

            KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput2);

            lockStdOut.lock();
            std::cout << strOutput2;
            lockStdOut.unlock();

            mapThreads.erase(threadId);
            });
        });  

    KeplerApplication::s_threadIdsToKill.safe_use([] (KeplerSynchronizedSetWrapper<std::thread::id>::KeplerSynchronizedSetType &setThreadsToKill)
        {
        setThreadsToKill.clear();
        }); 
    }

void KeplerFrame::createNewThreadsIfNeeded()
    {
    if (KeplerApplication::s_bool_endAllThreads == false)
        {
        // Access the threads object inside a critical section

        KeplerApplication::s_threads.safe_use([] (KeplerSynchronizedMapWrapper<std::thread::id, std::shared_ptr<std::thread>>::KeplerSynchronizedMapType &mapThreads) 
            {
            const int intThreadCount = mapThreads.size();
                
            if (intThreadCount < MAX_THREADS)
                {
                std::stringstream stringStreamOutput1;
                stringStreamOutput1 << "Number of threads in map: " << mapThreads.size() << std::endl;
                const std::string strOutput1 = stringStreamOutput1.str();

                KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput1);

                std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();
                std::cout << strOutput1;
                lockStdOut.unlock();

                for (const unsigned int i : boost::irange(intThreadCount,
                                                          MAX_THREADS)) 
                    {
                    std::shared_ptr<std::thread> ptr_newThread(new std::thread(threadFunction, i));

                    // Is this safe? Do we now have two ref-counted objects pointing to the thread but each with a count of 1?

                    mapThreads.insert(KeplerSynchronizedMapWrapper<std::thread::id, std::shared_ptr<std::thread>>::KeplerSynchronizedMapType::value_type(ptr_newThread->get_id(),
                                                                                                                                                         ptr_newThread));
                    }

                std::stringstream stringStreamOutput2;
                stringStreamOutput2 << "Number of threads in map is NOW: " << mapThreads.size() << std::endl;
                const std::string strOutput2 = stringStreamOutput2.str();

                KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLogQueue(strOutput2);

                lockStdOut.lock();
                std::cout << strOutput2;
                lockStdOut.unlock();
                }
            });
        }
    }

void KeplerFrame::OnTimer(wxTimerEvent &e)
    {
    m_uintTimerCounter++;

    killAndJoinThreadsIfNeeded();
    createNewThreadsIfNeeded();

    addTextToTextCtrlApplicationWideLogFromQueue();
    }