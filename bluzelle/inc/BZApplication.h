#ifndef BZAPPLICATION_H
#define BZAPPLICATION_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif



class BZApplication : public wxApp {
public:
    virtual bool OnInit();
};

#endif //BZAPPLICATION_H
