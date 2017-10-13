#include "BZRootFrame.h"
#include "bluzelleLogo.xpm.h"
#include "BZApplicationLogListView.h"
#include "BZEthereumAddressDialog.h"
#include "BZTask.h"
#include "EthereumApi.h"

#include <iostream>
#include <wx/wxprec.h>
#include <wx/stdpaths.h>
#include <wx/dialog.h>
#include <array>
#include <string>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#define MAIN_WINDOW_TIMER_PERIOD_MILLISECONDS 1000
#define CQUEUE_TIMER_PERIOD_MILLISECONDS 100
#define CREATE_NEW_DEFICIT_THREADS_PROBABILITY_PERCENTAGE 0.5


BEGIN_EVENT_TABLE(BZRootFrame, wxFrame)
    EVT_IDLE(BZRootFrame::OnIdle)
    EVT_CLOSE(BZRootFrame::OnClose)
END_EVENT_TABLE()

enum {
    ID_Hello = 1
};


CQueue<std::string> BZRootFrame::s_messageQueue;
uint64_t            BZRootFrame::s_loopCount = 0;
BZRootFrame*        BZRootFrame::s_instance = nullptr;
std::mutex          BZRootFrame::s_rootMutex;

void BZRootFrame::MenuInit() {
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(
            ID_Hello,
            "&Hello...\tCtrl-H",
            "Help string shown in status bar for this menu item");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar(menuBar);
}

void BZRootFrame::StatusBarInit() {
    CreateStatusBar();
    SetStatusText("Welcome to Kepler!");
}

BZRootFrame::BZRootFrame()
        : wxFrame(nullptr,
                  wxID_ANY,
                  "Kepler TestNet Simulator - ",
                  wxDefaultPosition,
                  wxDefaultSize,
                  wxDEFAULT_FRAME_STYLE),
          m_timerIdle(this,
                      wxID_ANY) {

    // TODO: RHN - Refactor this class...
    //      - Create sub classes for default static text
    //      - Create a factory for the list views.
    //      - use the size classes for layout.

    wxASSERT(nullptr == BZRootFrame::s_instance);
    BZRootFrame::s_instance = this; // TODO this is just bad...


    SetBackgroundColour(wxColour(59, 112, 140, wxALPHA_OPAQUE));
    MenuInit();
    StatusBarInit();
    Bind(wxEVT_MENU, &BZRootFrame::OnHello, this, ID_Hello);
    Bind(wxEVT_MENU, &BZRootFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &BZRootFrame::OnExit, this, wxID_EXIT);

    wxInitAllImageHandlers();

    wxBitmap bluzelleBitmap(ec9859b6a7014f63e9469a70eaced40b);
    //bluzelleBitmap.Rescale(wxSize(200,40).GetWidth(),wxSize(200,40).GetHeight());

    _bluzelleLogo = new wxStaticBitmap(
            this,
            wxID_ANY,
            bluzelleBitmap,
            wxPoint(15, 15),
            wxSize(200, 20),
            wxBORDER_STATIC,
            wxString("bluzelleBanner")
    );

//
//    //const wxBitmap bluzelleBitmap(ec9859b6a7014f63e9469a70eaced40b);
//    const wxBitmap bluzelleBitmap = wxImage(_T("../../bluzelle/assets/bluzelleLogo.png")).Rescale(wxSize(200,40).GetWidth(),wxSize(200,40).GetHeight());
//    //auto bluzelleLogo = new wxStaticBitmap(
//    wxStaticBitmap* bluzelleLogo = new wxStaticBitmap(
//            this,
//            wxID_ANY,
//            bluzelleBitmap,
//            wxPoint(15, 15),
//            wxSize(200,40),
//            wxBORDER_STATIC,
//            wxString("bluzelleBanner")
//    );
//

    // Ask user to provide ETH address to use on Ropsten network.
    auto addressDialog = make_unique<BZEthereumAddressDialog>(wxT("Please provide ETH address to use on Ropsten network"));
    addressDialog->ShowModal();
    ethereumAddress = addressDialog->GetAddress();
    addressDialog->Destroy();

    ethereumAddress = "0x006eae72077449caca91078ef78552c0cd9bce8f";

    auto api = make_unique<EthereumApi>(ethereumAddress);
    double balance = 0.0;
    {
        wxBusyCursor wait;
        balance = api->tokenBalance(tokenAddress);
    }

    if (balance < 0.0) { // API error. api.etherscan.io is down?
        auto message = boost::format("Unable to retrieve BLZ token balance for address %s") % tokenAddress;
        wxMessageBox(boost::str(message), "Token Balance", wxOK | wxICON_ERROR, this);
        Close(true);
        return;
    }

    if (balance / tokenDenomination < expectedTokenBalance) {
        auto message = boost::format("Your (%s) BLZ balance is %d\r\nwhich is lower than expected %d.\r\nPlease update your balance before running Kepler.") % tokenAddress % balance % expectedTokenBalance;
        wxMessageBox(boost::str(message), "Insufficient Token Balance", wxOK | wxICON_ERROR, this);
        Close(true);
        return;
    }


    nodeListTitle = new BZStaticText(
            this,
            wxString("Node List"),
            wxPoint(20, 75));

    notApplicableTitle = new BZStaticText(
            this,
            wxString("N/A"),
            wxPoint(240, 75));

    logTitle = new BZStaticText(
            this,
            wxString("Application-wide Log"),
            wxPoint(22, 525));

    nodeListView = new BZNodeListView(
            this,
            wxPoint(20, 100),
            wxSize(210, 415)
    );

    leftTopListView = new wxListView(
            this,
            wxID_ANY,
            wxPoint(240, 100),
            wxSize(210, 200),
            wxLC_REPORT,
            wxDefaultValidator,
            "top"
    );

    leftMiddleListView = new wxListView(
            this,
            wxID_ANY,
            wxPoint(240, 315),
            wxSize(210, 200),
            wxLC_REPORT,
            wxDefaultValidator,
            "middle"
    );


    logListView = new BZApplicationLogListView(
            this,
            wxPoint(20, 550),
            wxSize(800, 200));

    m_threadManager = &ThreadManager::GetInstance();

    vFrame = new wxFrame(nullptr, wxID_ANY, "test", wxPoint(20, 20), wxSize(500, 500));

    vFrame->SetBackgroundColour(wxColour(95, 117, 85));
    vFrame->Show(true);

    this->visualizationPanel = new BZVisualizationPanel(vFrame);
    this->visualizationPanel->Update();
    this->visualizationPanel->dataSource = m_threadManager;

    this->vFrame->Refresh(true);
    this->visualizationPanel->Refresh(true);

    m_timerIdle.Start(MAIN_WINDOW_TIMER_PERIOD_MILLISECONDS);
    Connect(
            m_timerIdle.GetId(),
            wxEVT_TIMER,
            wxTimerEventHandler(BZRootFrame::OnTimer),
            nullptr,
            this
    );
}

void BZRootFrame::OnExit(wxCommandEvent &event)
{
    wxUnusedVar(event);
    delete leftMiddleListView;
    delete leftTopListView;
    delete nodeListView;
    delete logTitle;
    delete notApplicableTitle;
    delete nodeListTitle;
    delete _bluzelleLogo;
    delete visualizationPanel;
    delete vFrame;
    Close(true);
}

void BZRootFrame::OnAbout(wxCommandEvent &event)
{
    wxUnusedVar(event);
    BZRootFrame::pushMessage("log","BZRootFrame::OnAbout");
    wxMessageBox("This is a wxWidgets' Hello world sample",
                 "About Hello World", wxOK | wxICON_INFORMATION);
}

void BZRootFrame::OnHello(wxCommandEvent &event)
{
    wxUnusedVar(event);
    std::cout << "Hello world from Bluzelle!:" << event.GetId() << "\n";
    wxLogMessage("Hello world from Bluzelle!");
}

void BZRootFrame::OnIdle(wxIdleEvent &event)
{
    wxUnusedVar(event);
    //BZRootFrame::pushMessage("log","BZRootFrame::OnIdle(wxIdleEvent &event)");
}

void BZRootFrame::OnClose(wxCloseEvent &event)
{
    wxUnusedVar(event);
    BZRootFrame::pushMessage("log","BZRootFrame::OnClose(wxIdleEvent &event)");
    Destroy();
}

void BZRootFrame::OnTimer(wxTimerEvent &event)
{
    wxUnusedVar(event);
    BZRootFrame::s_loopCount++; // 969 main.cpp

    cout << "begin BZRootFrame::OnTimer(wxTimerEvent &event)" <<  BZRootFrame::s_loopCount << " count: " << BZRootFrame::s_loopCount <<"\n" ;

    m_threadManager->killAndJoinThreadsIfNeeded();
    bool noThreads = false;
    ThreadManager::s_threads.safeUse(
            [
                    &noThreads
            ](
                    BZSyncMapWrapper<std::thread::id, ThreadData>::BZSyncMapType &mapThreads

            )
                {
                if(mapThreads.empty())
                    {
                    noThreads = true;
                    }
                }
    ); // 983


    if(noThreads)
        {
        m_threadManager->createNewThreadsIfNeeded();
        }
    else
        {
        if((((double)rand()/(double)RAND_MAX) * 100.0) <= CREATE_NEW_DEFICIT_THREADS_PROBABILITY_PERCENTAGE) // TODO USE Boostrand.
            {
            m_threadManager->createNewThreadsIfNeeded();
            }
        }


    // After doing all of the work, let's get the messages and put
    // them in the right places
    UpdateAppLogControl();

    std::cout << " ** BZRootFrame end-> thread count: [" << m_threadManager->s_threads.GetSize() << "] \n";
}


void BZRootFrame::UpdateAppLogControl()
{
    cout << "===== UpdateAppLogControl   " << BZRootFrame::s_messageQueue.empty() << "\n";


    while (!BZRootFrame::s_messageQueue.empty())
        {
        string msg(s_messageQueue.front());
        s_messageQueue.pop();
        vector<string> items;
        boost::split(items, msg, boost::is_any_of("|"));

        cout << " **** " << items[0] << "  " << items[1] << "\n";

        if( 0 == items[0].compare("log"))
            {
            addTextToLog(items[1].c_str());
            }
        }
}

void BZRootFrame::addTextToLog(const char* text)
{
    this->logListView->AddLogText(text);
}
