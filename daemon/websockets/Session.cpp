#include "Session.h"

#include <sstream>
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
Session::on_accept(
        boost::system::error_code ec)
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

    std::string response;
    if(buffer_.size() > 0)
        {
        response = process_json_string(buffer_);
        }
    auto b = boost::asio::buffer(fix_json_numbers(response));
    ws_.async_write(
            b,
            std::bind(
                    &Session::on_write,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2));
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

std::string
Session::process_json_string(
        const boost::beast::multi_buffer &buffer
)
{
    std::string response;
    try
        {
        std::stringstream ss;
        ss << boost::beast::buffers(buffer.data());
        pt::ptree request;
        pt::read_json( ss, request);
        auto command = request.get<std::string>("cmd");
        seq_ = request.get<size_t>("seq");
        // TODO: When we start building the services we need to replace this with
        // the command strategy pattern
        if("ping" == command)
            {
            request.put( "cmd", "pong");
            }
        else
            {
            request.put( "cmd", "error");
            request.put( "data", "Command not found.");
            }

        ss.str("");
        pt::write_json(ss, request);
        response = ss.str();
        }
    catch(const std::exception &e)
        {
        std::cerr << "Error processing command: [" << e.what() << "]" << std::endl;
        }
    return response;
}
