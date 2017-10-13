#ifndef KEPLER_BZETHEREUMADDRESSDIALOG_H
#define KEPLER_BZETHEREUMADDRESSDIALOG_H

#include <wx/wx.h>

#include <string>
#include <memory>

using std::unique_ptr;
using std::string;

class BZEthereumAddressDialog : public wxDialog {
public:
    BZEthereumAddressDialog(const wxString& title);
    wxString GetAddress();

private:
    //wxString ethereumAddress;

    unique_ptr<wxPanel> panel;
    unique_ptr<wxStaticBox> label;
    unique_ptr<wxTextValidator> hexValidator;
    unique_ptr<wxTextCtrl> addressTextCtrl;
    unique_ptr<wxButton> okButton, closeButton;
};


#endif //KEPLER_BZETHEREUMADDRESSDIALOG_H
