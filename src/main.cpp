#include <boost/locale.hpp>

#include <iostream>

#include <ctime>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/listctrl.h>



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

KeplerFrame::KeplerFrame()
            :wxFrame(NULL, 
                     wxID_ANY, 
                     "Kepler TestNet Simulator")
    {
    wxMenu *menuFile = new wxMenu;

    menuFile->Append(ID_Kepler, 
                     "&Welcome...\tCtrl-H",
                     "Change this text to something appropriate");
    
    menuFile->AppendSeparator();
    
    menuFile->Append(wxID_EXIT);
    
    wxMenu *menuHelp = new wxMenu;
    
    menuHelp->Append(wxID_ABOUT);
    
    wxMenuBar *menuBar = new wxMenuBar;
    
    menuBar->Append(menuFile, 
                    "&File");
    
    menuBar->Append(menuHelp, 
                    "&Help");
    
    SetMenuBar(menuBar);
    
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


    wxBoxSizer *boxSizerApplication = new wxBoxSizer(wxVERTICAL);



    wxBoxSizer *boxSizerTop = new wxBoxSizer(wxHORIZONTAL);



    boxSizerApplication->Add(
        boxSizerTop,
        4,                // make vertically unstretchable
        wxEXPAND | wxALL ); // no border and centre horizontally


    boxSizerApplication->Add(
        new wxTextCtrl( this, -1, "My text.", wxDefaultPosition, wxSize(100,60), wxTE_MULTILINE),
        1,           // make horizontally unstretchable
        wxEXPAND | wxALL,       // make border all around (implicit top alignment)
        10 );        // set border width to 10




    wxListView* listViewNodes = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(250, 200));

    listViewNodes->AppendColumn("Node Cardinality #");
    listViewNodes->AppendColumn("Node Physical Hash Id");

    // Add three items to the list

    listViewNodes->InsertItem(0, "1");
    listViewNodes->SetItem(0, 1, "Alfa");
    listViewNodes->InsertItem(1, "2");
    listViewNodes->SetItem(1, 1, "Bravo");
    listViewNodes->InsertItem(2, "3");
    listViewNodes->SetItem(2, 1, "Charlie");



    boxSizerTop->Add(
        listViewNodes,
        1,            // make horizontal weight 1
        wxEXPAND |    // make horizontally stretchable
        wxALL,        //   and make border all around
        10 );         // set border width to 10



    wxBoxSizer *boxSizerSelectedThread = new wxBoxSizer(wxVERTICAL);

    wxListView* listViewNodeAttributes = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(250, 200));



    listViewNodeAttributes->AppendColumn("Attribute Name");
    listViewNodeAttributes->AppendColumn("Attribute Value");

    // Add three items to the list

    listViewNodeAttributes->InsertItem(0, "Name");
    listViewNodeAttributes->SetItem(0, 1, "Alfa");
    listViewNodeAttributes->InsertItem(1, "Weight");
    listViewNodeAttributes->SetItem(1, 1, "5");
    listViewNodeAttributes->InsertItem(2, "Address");
    listViewNodeAttributes->SetItem(2, 1, "Sesame Street");



    boxSizerSelectedThread->Add(
        listViewNodeAttributes,
        4,           // make horizontally unstretchable
        wxEXPAND | wxALL,       // make border all around (implicit top alignment)
        10 );        // set border width to 10



    boxSizerSelectedThread->Add(
        new wxTextCtrl( this, -1, "My text.", wxDefaultPosition, wxSize(100,60), wxTE_MULTILINE),
        1,           // make horizontally unstretchable
        wxEXPAND | wxALL,       // make border all around (implicit top alignment)
        10 );        // set border width to 10



    boxSizerTop->Add(
        boxSizerSelectedThread,
        1,                // make vertically unstretchable
        wxEXPAND | wxALL ); // no border and centre horizontally



    SetSizerAndFit(boxSizerApplication); // use the sizer for layout and size window
                              // accordingly and prevent it from being resized
                              // to smaller size
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