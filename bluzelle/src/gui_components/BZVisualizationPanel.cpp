#include "BZVisualizationPanel.h"
#include <wx/wxprec.h>
#include <wx/image.h>
#include <wx/listctrl.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

BEGIN_EVENT_TABLE(BZVisualizationPanel, wxPanel)
                EVT_PAINT(BZVisualizationPanel::paintEvent)
END_EVENT_TABLE()

BZVisualizationPanel::BZVisualizationPanel(wxFrame* parent):wxPanel(parent) {
    this->m_windowId = wxID_ANY;
    this->SetPosition(wxPoint( 460, 100));
    this->SetSize(wxSize(400, 650));
    this->SetName("visualization");
}

void BZVisualizationPanel::paintEvent(wxPaintEvent &evt) {
    wxPaintDC dc(this);
    render(dc);
}

void BZVisualizationPanel::paintNow()
{
    wxClientDC dc(this);
    render(dc);
}

const wxColour foreGround = wxColour(69,233,38);

void drawNode(wxDC& dc, const char* txt, const int x, const int y) {
    wxPen pen = dc.GetPen();
    pen.SetColour(foreGround);
    pen.SetWidth(3);
    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawCircle(x, y, 6);
    dc.SetTextForeground(foreGround);
    dc.DrawText(txt, x + 5, y + 5);
}

void BZVisualizationPanel::render(wxDC&  dc)
{
    uint8_t nodecount = 8;
    wxSize sz = this->GetSize();

    wxPoint c = wxPoint(sz.GetWidth()/2,sz.GetHeight()/2);
    drawNode(dc, "0x1EF64A", c.x, c.y);
}
