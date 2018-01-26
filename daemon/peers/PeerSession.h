#ifndef BLUZELLE_PEERSESSION_H
#define BLUZELLE_PEERSESSION_H

#include <string>
#include <boost/beast.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_service.hpp>

using namespace std;
using namespace boost;

class
PeerSession :
    public
    std::enable_shared_from_this<PeerSession>
{
private:
    beast::websocket::stream<asio::ip::tcp::socket> ws_;
    asio::io_service::strand                        strand_;
    beast::multi_buffer                             buffer_;
    function<string(const string&)>                 handler_ = nullptr;
    bool                                            schedule_read_; // Sometimes we don't expect response, so set this flag if no on_read() need to be called.

public:
    explicit
    PeerSession
        (
            beast::websocket::stream<asio::ip::tcp::socket> ws
        );

    ~PeerSession();

    void
    run();

    void
    set_request_handler
        (
            function<string(const string&)> request_handler
        );

    void
    on_accept
        (
            system::error_code ec
        );

    void
    do_read();

    void
    on_read
        (
            system::error_code ec,
            size_t bytes_transferred
        );

    void
    on_write
        (
            system::error_code ec,
            size_t bytes_transferred
        );

    void
    write_async
        (
            const string& message
        );

    void
    needs_read(
        bool n=true
    )
    {
        schedule_read_ = n;
    }
};

#endif //BLUZELLE_PEERSESSION_H
