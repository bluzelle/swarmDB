#include <boost/locale.hpp>

#include <iostream>

#include <ctime>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

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
                     "Kepler TestNet Node")
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