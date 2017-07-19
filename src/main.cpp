#include <boost/locale.hpp>

#include <iostream>

#include <ctime>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/listctrl.h>

#include <thread>



class KeplerApplication: public wxApp
    {
    public:

        virtual bool OnInit();
    };

class KeplerFrame: public wxFrame
    {
    public:
    
        KeplerFrame();
    
    private:
    
        void OnKepler(wxCommandEvent& event);
        void OnExit(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);

        wxMenu *m_ptr_menuFile;
        wxMenu *m_ptr_menuHelp;

        wxMenuBar *m_ptr_menuBar;

        wxBoxSizer *boxSizerApplication;
        wxBoxSizer *boxSizerTop;
        wxBoxSizer *boxSizerSelectedThread;
        wxBoxSizer *boxSizerThreadMessages;

        wxTextCtrl *textCtrlApplicationWideLog;
        wxTextCtrl *textCtrlThreadLog;        

        wxListView* listViewNodes;
        wxListView* listViewNodeKeyValuesStore;
        wxListView* listViewNodeInbox;
        wxListView* listViewNodeOutbox; 
        wxListView* listViewNodeAttributes;

        wxStaticText *staticTextThreadIdentifier; 
    };

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

   void call_from_thread() {
         std::cout << "Hello, World" << std::endl;
   }
 

KeplerFrame::KeplerFrame()
            :wxFrame(NULL, 
                     wxID_ANY, 
                     "Kepler TestNet Simulator")
    {

std::thread t1(call_from_thread);
t1.join();

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



    boxSizerApplication = new wxBoxSizer(wxVERTICAL);



    boxSizerTop = new wxBoxSizer(wxHORIZONTAL);



    boxSizerApplication->Add(boxSizerTop,
                             4,                
                             wxEXPAND | 
                             wxALL ); 



    textCtrlApplicationWideLog = new wxTextCtrl(this, 
                                                -1, 
                                                "My text.", 
                                                wxDefaultPosition, 
                                                wxSize(100,60), 
                                                wxTE_MULTILINE);
    boxSizerApplication->Add(textCtrlApplicationWideLog,
                             0.5,
                             wxEXPAND | 
                             wxALL,       
                             10);



    listViewNodes = new wxListView(this, 
                                               wxID_ANY, 
                                               wxDefaultPosition, 
                                               wxSize(250, 200));

    listViewNodes->AppendColumn("Node Cardinality #");
    listViewNodes->AppendColumn("Node Physical Hash Id");

    // Add three items to the list

    listViewNodes->InsertItem(0, "1");
    listViewNodes->SetItem(0, 1, "Alfa");
    listViewNodes->InsertItem(1, "2");
    listViewNodes->SetItem(1, 1, "Bravo");
    listViewNodes->InsertItem(2, "3");
    listViewNodes->SetItem(2, 1, "Charlie");

    listViewNodes->SetColumnWidth(0, wxLIST_AUTOSIZE);
    listViewNodes->SetColumnWidth(1, wxLIST_AUTOSIZE);

    boxSizerTop->Add(listViewNodes,
                     1,
                     wxEXPAND | 
                     wxALL, 
                     10);



    boxSizerSelectedThread = new wxBoxSizer(wxVERTICAL);



    staticTextThreadIdentifier = new wxStaticText(this, 
                                                                wxID_ANY,
                                                                "Foo",
                                                                wxDefaultPosition,
                                                                wxDefaultSize,
                                                                wxALIGN_CENTRE);

    boxSizerSelectedThread->Add(staticTextThreadIdentifier,
                                0,
                                wxEXPAND | 
                                wxALL,
                                10);



    listViewNodeKeyValuesStore = new wxListView(this, 
                                                            wxID_ANY, 
                                                            wxDefaultPosition, 
                                                            wxSize(250, 200));

    listViewNodeKeyValuesStore->AppendColumn("Key");
    listViewNodeKeyValuesStore->AppendColumn("Value");

    // Add three items to the list

    listViewNodeKeyValuesStore->InsertItem(0, "id1");
    listViewNodeKeyValuesStore->SetItem(0, 1, "Alfa");
    listViewNodeKeyValuesStore->InsertItem(1, "address1");
    listViewNodeKeyValuesStore->SetItem(1, 1, "Sesame Street");
    listViewNodeKeyValuesStore->InsertItem(2, "DOB1");
    listViewNodeKeyValuesStore->SetItem(2, 1, "18/05/1976");

    listViewNodeKeyValuesStore->SetColumnWidth(0, wxLIST_AUTOSIZE);
    listViewNodeKeyValuesStore->SetColumnWidth(1, wxLIST_AUTOSIZE);

    boxSizerSelectedThread->Add(listViewNodeKeyValuesStore,
                                2,
                                wxEXPAND | 
                                wxALL,
                                10);



    boxSizerThreadMessages = new wxBoxSizer(wxHORIZONTAL);

    boxSizerSelectedThread->Add(boxSizerThreadMessages,
                                0.5,
                                wxEXPAND | 
                                wxALL );



    listViewNodeInbox = new wxListView(this, 
                                                   wxID_ANY, 
                                                   wxDefaultPosition, 
                                                   wxSize(250, 200));

    listViewNodeInbox->AppendColumn("Inbox Message");

    // Add three items to the list

    listViewNodeInbox->InsertItem(0, "id1");
    listViewNodeInbox->InsertItem(1, "address1");
    listViewNodeInbox->InsertItem(2, "DOB1");

    listViewNodeInbox->SetColumnWidth(0, wxLIST_AUTOSIZE);

    boxSizerThreadMessages->Add(listViewNodeInbox,
                                1,
                                wxEXPAND | 
                                wxALL,
                                10);



    listViewNodeOutbox = new wxListView(this, 
                                                    wxID_ANY, 
                                                    wxDefaultPosition, 
                                                    wxSize(250, 200));

    listViewNodeOutbox->AppendColumn("Outbox Message");

    // Add three items to the list

    listViewNodeOutbox->InsertItem(0, "id1");
    listViewNodeOutbox->InsertItem(1, "address1");
    listViewNodeOutbox->InsertItem(2, "DOB1");

    listViewNodeOutbox->SetColumnWidth(0, wxLIST_AUTOSIZE);

    boxSizerThreadMessages->Add(listViewNodeOutbox,
                                1,
                                wxEXPAND | 
                                wxALL,
                                10);



    listViewNodeAttributes = new wxListView(this, 
                                                        wxID_ANY, 
                                                        wxDefaultPosition, 
                                                        wxSize(250, 200));

    listViewNodeAttributes->AppendColumn("Attribute Name");
    listViewNodeAttributes->AppendColumn("Attribute Value");

    // Add three items to the list

    listViewNodeAttributes->InsertItem(0, "Name");
    listViewNodeAttributes->SetItem(0, 1, "Alfa");
    listViewNodeAttributes->InsertItem(1, "Weight");
    listViewNodeAttributes->SetItem(1, 1, "5");
    listViewNodeAttributes->InsertItem(2, "Address");
    listViewNodeAttributes->SetItem(2, 1, "Sesame Street");

    listViewNodeAttributes->SetColumnWidth(0, wxLIST_AUTOSIZE);
    listViewNodeAttributes->SetColumnWidth(1, wxLIST_AUTOSIZE);

    boxSizerSelectedThread->Add(listViewNodeAttributes,
                                0.25,
                                wxEXPAND | 
                                wxALL,
                                10);



    textCtrlThreadLog = new wxTextCtrl(this, 
                                                   -1, 
                                                   "My text.", 
                                                   wxDefaultPosition, 
                                                   wxSize(100,60), 
                                                   wxTE_MULTILINE);

    boxSizerSelectedThread->Add(textCtrlThreadLog,
                                0.1,
                                wxEXPAND | 
                                wxALL,
                                10);



    boxSizerTop->Add(boxSizerSelectedThread,
                     1,
                     wxEXPAND | 
                     wxALL );



    SetSizerAndFit(boxSizerApplication);
    }

void KeplerFrame::OnExit(wxCommandEvent& event)
    {
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
    }

void KeplerFrame::OnKepler(wxCommandEvent& event)
    {
    wxLogMessage("Hello from Kepler!");
    }