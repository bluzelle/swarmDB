#include "BZVisualizationPanel.h"

//#include <math.h>
#include <wx/wxprec.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <algorithm>;
#include <boost/algorithm/string.hpp>

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

//const wxColour foreGround = wxColour(69,233,38);
const wxColour foreGround = wxColour(70,234,39);

void drawNode(wxDC& dc, const char* msg, const char* numMsgs, const int x, const int y) {
    wxPen pen = dc.GetPen();
    pen.SetColour(foreGround);
    pen.SetWidth(2);
    dc.SetPen(pen);
    dc.SetTextForeground(foreGround);

    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    dc.DrawCircle(x, y, 6);

    dc.DrawText(msg, x - 50, y + 6);

    char txt[256];
    snprintf(txt,256,"Messages: %s", numMsgs);

    dc.DrawText(txt, x - 50, y + 18);
}

void BZVisualizationPanel::render(wxDC&  dc)
{
    uint8_t nodecount = 8;
    wxSize sz = this->GetSize();
    wxPoint c = wxPoint(sz.GetWidth()/2,sz.GetHeight()/2);
    double r = 0.4 * std::min(sz.GetWidth(), sz.GetHeight());

    if(!_nodeData.empty()) {
        double dr = (2.0 * M_PI) / (float)_nodeData.size();
        double theta = 0.0;
        for(auto node : _nodeData)
            {
            double x = c.x + r * cos(theta);
            double y = c.y + r * sin(theta);

            std::vector<std::string> params;
            boost::split(params, node, boost::is_any_of("|"));

            drawNode(dc, params[0].c_str(), params[1].c_str(), x, y);
            theta += dr;
            }
        }

    std::string txt = std::to_string(_nodeData.size());
    txt.append(" Nodes");
    dc.DrawText(txt, 25, 25);

}
