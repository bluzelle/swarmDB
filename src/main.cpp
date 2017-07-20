#include "KeplerSynchronizedSet.hpp"

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



#define MAX_THREADS 1



class KeplerApplication: public wxApp
    {
    public:

        virtual bool OnInit();

        static std::unique_lock<std::mutex> getStdOutLock();

        static unsigned int sleepRandomMilliseconds(const unsigned int uintMaximumMilliseconds);

        static KeplerSynchronizedSet<std::shared_ptr<std::thread>> s_threads;

        static std::mutex s_stdOutMutex;              
    };



KeplerSynchronizedSet<std::shared_ptr<std::thread>> KeplerApplication::s_threads;

std::mutex KeplerApplication::s_stdOutMutex;



std::unique_lock<std::mutex> KeplerApplication::getStdOutLock()
    {
    std::unique_lock<std::mutex> mutexLock(s_stdOutMutex,
                                           std::defer_lock);

    mutexLock.lock();

    return mutexLock;
    }



unsigned int KeplerApplication::sleepRandomMilliseconds(const unsigned int uintMaximumMilliseconds)
    {
    typedef unsigned long long u64;

    u64 u64useconds;
    struct timeval tv;

    gettimeofday(&tv,NULL);
    u64useconds = (1000000*tv.tv_sec) + tv.tv_usec;

    std::srand(u64useconds); // use current time as seed for random generator

    int intRandomVariable = std::rand();

    unsigned int uintActualMillisecondsToSleep = intRandomVariable % uintMaximumMilliseconds;

    std::this_thread::sleep_for(std::chrono::milliseconds(uintActualMillisecondsToSleep));      

    return uintActualMillisecondsToSleep;      
    }



class KeplerFrame: public wxFrame
    {
    public:
    
        KeplerFrame();

        std::unique_lock<std::mutex> getTextCtrlApplicationWideLogLock();

        void addTextToTextCtrlApplicationWideLog(const std::string &str);

        static KeplerFrame *s_ptr_global;

    private:
    
        void OnKepler(wxCommandEvent& event);
        void OnExit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);

        wxMenu *m_ptr_menuFile;
        wxMenu *m_ptr_menuHelp;

        wxMenuBar *m_ptr_menuBar;

        wxBoxSizer *m_ptr_boxSizerApplication;
        wxBoxSizer *m_ptr_boxSizerTop;
        wxBoxSizer *m_ptr_boxSizerSelectedThread;
        wxBoxSizer *m_ptr_boxSizerThreadMessages;

        wxTextCtrl *m_ptr_textCtrlApplicationWideLog;
        wxTextCtrl *m_ptr_textCtrlThreadLog;        

        wxListView *m_ptr_listViewNodes;
        wxListView *m_ptr_listViewNodeKeyValuesStore;
        wxListView *m_ptr_listViewNodeInbox;
        wxListView *m_ptr_listViewNodeOutbox; 
        wxListView *m_ptr_listViewNodeAttributes;

        wxStaticText *m_ptr_staticTextThreadIdentifier; 

        std::mutex m_textCtrlApplicationWideLogMutex;
    };



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
    std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

    std::thread::id myThreadId = std::this_thread::get_id();

    std::cout << "Hello. This is thread #: " << i << " with id: " << myThreadId << std::endl;        
    }

void threadLifeCycle(const unsigned int i)
    {
    while (true)
        {
        unsigned int uintActualMillisecondsToSleep = KeplerApplication::sleepRandomMilliseconds(1000);

        std::thread::id myThreadId = std::this_thread::get_id();

        std::stringstream stringStreamOutput;

        stringStreamOutput << "Thread #: " 
                  << i 
                  << " awakens after " 
                  << uintActualMillisecondsToSleep 
                  << "ms and then goes back to sleep with id: " 
                  << myThreadId 
                  << std::endl;      

        std::string strOutput = stringStreamOutput.str();          

        KeplerFrame::s_ptr_global->addTextToTextCtrlApplicationWideLog(strOutput);

        std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

        std::cout << strOutput;
        }
    }

void threadFunction(const unsigned int i) 
    {
    threadIntroduction(i);

    threadLifeCycle(i);
    }
 


KeplerFrame::KeplerFrame()
            :wxFrame(NULL, 
                     wxID_ANY, 
                     "Kepler TestNet Simulator")
    {
    KeplerFrame::s_ptr_global = this;

    if ( __cplusplus == 201103L ) std::cout << "C++11\n" ;
 
    else if( __cplusplus == 19971L ) std::cout << "C++98\n" ;
 
    else std::cout << "pre-standard C++\n" ;



    std::cout << "Main thread id: " << std::this_thread::get_id() << "\n";



    for (const unsigned int i : boost::irange(0,
                                              MAX_THREADS)) 
        {
        std::shared_ptr<std::thread> ptr_newThread(new std::thread(threadFunction, i));

        // Is this safe? Do we now have two ref-counted objects pointing to the thread but each with a count of 1?

        const bool boolInsertionResult = KeplerApplication::s_threads.safe_insert(ptr_newThread);

        std::unique_lock<std::mutex> lockStdOut = KeplerApplication::getStdOutLock();

        std::cout << "Started thread" << "\n";

        // We don't do a join on these threads -- might want to when the program quits?
        }



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
                             4,                
                             wxEXPAND | 
                             wxALL ); 



    m_ptr_textCtrlApplicationWideLog = new wxTextCtrl(this, 
                                                -1, 
                                                "My text.", 
                                                wxDefaultPosition, 
                                                wxSize(100,60), 
                                                wxTE_MULTILINE | 
                                                wxTE_READONLY |
                                                wxHSCROLL);
    m_ptr_boxSizerApplication->Add(m_ptr_textCtrlApplicationWideLog,
                             0.5,
                             wxEXPAND | 
                             wxALL,       
                             10);

    m_ptr_listViewNodes = new wxListView(this, 
                                               wxID_ANY, 
                                               wxDefaultPosition, 
                                               wxSize(250, 200));

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
                                                                "Foo",
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
                                                            wxSize(250, 200));

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
                                                   wxSize(250, 200));

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
                                                    wxSize(250, 200));

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
                                                        wxSize(250, 200));

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
                                                   "My text.", 
                                                   wxDefaultPosition, 
                                                   wxSize(100,60), 
                                                   wxTE_MULTILINE);

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
    }

void KeplerFrame::OnExit(wxCommandEvent& event)
    {
int myints[] = {75,23,65,42,13};
  std::set<int> myset (myints,myints+5);

  std::cout << "myset contains:";
  for (std::set<int>::iterator it=myset.begin(); it!=myset.end(); ++it)
    std::cout << ' ' << *it;


    Close(true);
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

    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    addTextToTextCtrlApplicationWideLog("Batman");
    }

void KeplerFrame::OnKepler(wxCommandEvent& event)
    {
    wxLogMessage("Hello from Kepler!");
    }

std::unique_lock<std::mutex> KeplerFrame::getTextCtrlApplicationWideLogLock()
    {
    std::unique_lock<std::mutex> mutexLock(m_textCtrlApplicationWideLogMutex,
                                           std::defer_lock);

    mutexLock.lock();

    return mutexLock;
    }

void KeplerFrame::addTextToTextCtrlApplicationWideLog(const std::string &str)
    {
    wxMutexGuiEnter();

    std::unique_lock<std::mutex> lockTextCtrlApplicationWideLog = KeplerFrame::getTextCtrlApplicationWideLogLock();   

    // std::ostream stream(m_ptr_textCtrlApplicationWideLog);

    // stream << str << std::endl;

    // stream.flush();

    wxMutexGuiLeave();    
    }