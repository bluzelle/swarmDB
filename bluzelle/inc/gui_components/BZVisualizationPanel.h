#ifndef KEPLER_BZVISUALIZATIONPANEL_H
#define KEPLER_BZVISUALIZATIONPANEL_H

#include <wx/wxprec.h>
#include <wx/image.h>
#include <wx/listctrl.h>

#include "ThreadManager.h"
#include "BZDataSource.h"

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

class BZVisualizationPanel : public wxPanel
{
protected:
    DECLARE_EVENT_TABLE()

public:
    BZVisualizationPanel(wxFrame* parent);
    void paintEvent(wxPaintEvent& evt);
    void paintNow();
    void render(wxDC& dc);

    BZDataSource* dataSource;

};

#endif //KEPLER_BZVISUALIZATIONPANEL_H
