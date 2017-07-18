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

    wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL);

    wxListView* listView = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(250, 200));

    listView->AppendColumn("Node Cardinality #");
    listView->AppendColumn("Node Physical Hash Id");

    // Add three items to the list
    listView->InsertItem(0, "1");
    listView->SetItem(0, 1, "Alfa");
    listView->InsertItem(1, "2");
    listView->SetItem(1, 1, "Bravo");
    listView->InsertItem(2, "3");
    listView->SetItem(2, 1, "Charlie");

    topsizer->Add(
        listView,
        1,            // make horizontal weight 1
        wxEXPAND |    // make horizontally stretchable
        wxALL,        //   and make border all around
        10 );         // set border width to 10

    wxBoxSizer *button_sizer = new wxBoxSizer(wxVERTICAL);

    wxListView* listView2 = new wxListView(this, wxID_ANY, wxDefaultPosition, wxSize(250, 200));

    listView2->AppendColumn("Attribute Name");
    listView2->AppendColumn("Attribute Value");

    // Add three items to the list
    listView2->InsertItem(0, "Name");
    listView2->SetItem(0, 1, "Alfa");
    listView2->InsertItem(1, "Weight");
    listView2->SetItem(1, 1, "5");
    listView2->InsertItem(2, "Address");
    listView2->SetItem(2, 1, "Sesame Street");


    button_sizer->Add(
        listView2,
        4,           // make horizontally unstretchable
        wxEXPAND | wxALL,       // make border all around (implicit top alignment)
        10 );        // set border width to 10
    button_sizer->Add(
        new wxTextCtrl( this, -1, "My text.", wxDefaultPosition, wxSize(100,60), wxTE_MULTILINE),
        1,           // make horizontally unstretchable
        wxEXPAND | wxALL,       // make border all around (implicit top alignment)
        10 );        // set border width to 10

    topsizer->Add(
        button_sizer,
        1,                // make vertically unstretchable
        wxEXPAND | wxALL ); // no border and centre horizontally
    SetSizerAndFit(topsizer); // use the sizer for layout and size window
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