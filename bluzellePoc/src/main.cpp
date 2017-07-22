#include "KeplerFrame.hpp"
#include "KeplerApplication.hpp"

BEGIN_EVENT_TABLE(KeplerFrame, wxFrame)
   EVT_IDLE(KeplerFrame::OnIdle)
   EVT_CLOSE(KeplerFrame::OnClose)
END_EVENT_TABLE()


wxIMPLEMENT_APP(KeplerApplication);



