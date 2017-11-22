#include "Session.h"

#include <boost/date_time/posix_time/posix_time.hpp>

// Report a failure
void
fail(boost::system::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

Session::Session
        (
                tcp::socket socket
        )
        : ws_(std::move(socket)), strand_(ws_.get_io_service())
{

}

// Start the asynchronous operation
void
Session::run()
{
    // Accept the websocket handshake
    ws_.async_accept(
            strand_.wrap(std::bind(
                    &Session::on_accept,
                    shared_from_this(),
                    std::placeholders::_1)));
}

void
Session::on_accept(boost::system::error_code ec)
{
    if (ec)
        return fail(ec, "accept");

    // Read a message
    do_read();
}

void
Session::do_read()
{
    // Read a message into our buffer
    ws_.async_read(
            buffer_,
            strand_.wrap(std::bind(
                    &Session::on_read,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
}

void
Session::on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if (ec == websocket::error::closed)
        return;

    if (ec)
        fail(ec, "read");

    std::stringstream ss;
    ss << boost::beast::buffers(buffer_.data());

    pt::ptree request;
    pt::read_json(ss, request);

    ///////////////////////////////////////////////////////////////////////////
    // TODO: this is where requests are transformed
    std::ostringstream oss;
    pt::write_json(oss,request);

    std::string response = oss.str();

    ///////////////////////////////////////////////////////////////////////////
    ws_.async_write(
            boost::asio::buffer(response),
            std::bind(
                    &Session::on_write,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));


    state_ = set_state(request);
    seq = request.get<int>("seq");
}

void
Session::on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return fail(ec, "write");

    // Clear the buffer
    buffer_.consume(buffer_.size());

//    if (state_ == SessionState::Started)
//        {
//        auto request = boost::format("{\"seq\": %d}") % ++seq;
//        std::string response = services_("getAllNodes", boost::str(request));
//
//        if (response.length() > 0) // Send updated nodes status.
//            {
//            std::cout << " ******* " << std::endl << response << std::endl;
//            ws_.write(boost::asio::buffer(response));
//            state_ = SessionState::Started; // Send all nodes once.
//            }
//        }

    buffer_.consume(buffer_.size());

    // Do another read
    do_read();
}

SessionState
Session::set_state(
        const pt::ptree &request)
{
    if (request.get<std::string>("cmd") == "getMaxNodes")
        return SessionState::Starting;

    if (request.get<std::string>("cmd") == "getMinNodes")
        return SessionState::Started;

    return state_;
}

void
Session::write_async(boost::asio::const_buffers_1 b)
{
    ws_.async_write(
            b,
            std::bind(
                    &Session::on_write_async,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
}

void
Session::on_write_async(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return fail(ec, "write");

    buffer_.consume(buffer_.size());
}

void
Session::send_remove_nodes(
        const std::string &name)
{

    // {"cmd":"removeNodes","data":["0x07"]}
    auto f = boost::format("{\"cmd\":\"removeNodes\", \"data\":[\"%s\"]}") % name;
    std::string response = boost::str(f);

    if (response.length() > 0) // Send removed nodes.
        {
        write_async(boost::asio::buffer(response));
        }
}

void
Session::send_update_nodes(
        const std::string &name)
{

    // {"cmd":"updateNodes","data":[{"address":"0x0e"}]}
    auto f = boost::format("{\"cmd\":\"updateNodes\",\"data\":[{\"address\":\"%s\"}]}") % name;
    std::string response = boost::str(f);

    if (response.length() > 0)
        {
        std::cout << " ******* " << std::endl << response << std::endl;
        write_async(boost::asio::buffer(response));
        }
}

void
Session::send_message(
        const std::string &from, const std::string &to, const std::string &message)
{

    auto f = boost::format(
            R"({"cmd":"messages","data":[{"srcAddr":"%s", "dstAddr":"%s", "timestamp":"%s", "body":%s}]})")
             % from % to % timestamp() % message;

    std::string response = boost::str(f);

    std::cout << " ******* " << std::endl << response << std::endl;
    write_async(boost::asio::buffer(response));
}

void
Session::send_log(
        const std::string &name, int timer, int entry, const std::string &log)
{

    auto f = boost::format(
            R"({"cmd":"log","data":[{"name":%d, "timer_no":%d, "entry_no":%d, "timestamp":"%s", "message":"%s"}]})")
             % name % timer % entry % timestamp() % log;
    std::string response = boost::str(f);
    write_async(boost::asio::buffer(response));
}

std::string
Session::timestamp()
{
    static std::locale loc(
            std::wcout.getloc(),
            new boost::posix_time::time_facet("%Y-%m-%dT%H:%M:%sZ"));

    std::basic_stringstream<char> ss;

    ss.imbue(loc);
    ss << boost::posix_time::microsec_clock::universal_time();

    return ss.str();
}
