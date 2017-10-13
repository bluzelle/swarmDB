#include "BZEthereumAddressDialog.h"

#include <memory>


BZEthereumAddressDialog::BZEthereumAddressDialog(const wxString& title)
        : wxDialog(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(530, 160)) {

    panel = std::make_unique<wxPanel>(this);
    panel->SetSize(this->GetSize());

    label = std::make_unique<wxStaticBox>(
            panel.get(), wxID_ANY, wxT("ETH address (Ropsten network): 0x"),
            wxPoint(10, 20), wxDefaultSize);

    // HEX validator.
    hexValidator = std::make_unique<wxTextValidator>(wxFILTER_EMPTY | wxFILTER_INCLUDE_CHAR_LIST/*, &ethereumAddress*/);
    hexValidator->SetCharIncludes("0123456789abcdefABCDEF");

    addressTextCtrl = unique_ptr<wxTextCtrl>(new wxTextCtrl(
            panel.get(), wxID_ANY, wxT(""),
            wxPoint(label->GetPosition().x + label->GetSize().GetWidth() + 10, label->GetPosition().y), wxSize(290, 30),
            0L, *(hexValidator.get()), wxTextCtrlNameStr));

    addressTextCtrl->SetMaxLength(40);

    okButton = std::make_unique<wxButton>(
            panel.get(), wxID_OK, wxT("Ok"), wxPoint(180, 80), wxSize(70, 30));

    closeButton = std::make_unique<wxButton>(
            panel.get(), wxID_CANCEL, wxT("Close"),
            wxPoint(okButton->GetPosition().x + okButton->GetSize().GetWidth() + 10, okButton->GetPosition().y), wxSize(70, 30));

    Centre();
}

wxString BZEthereumAddressDialog:: GetAddress() {
    return "0x" + addressTextCtrl->GetValue();
}
