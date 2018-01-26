#include "CommandLineOptions.h"
#include "Listener.h"
#include "Node.h"
#include "TokenBalance.hpp"
#include "node/Singleton.h"
#include "node/DaemonInfo.h"
#include "Node.h"
#include "ParseUtils.h"
#include "WebSocketServer.h"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>


constexpr int s_uint_minimum_required_token_balance = 100;
constexpr char s_etherscan_api_token_envar_name[] = "ETHERSCAN_IO_API_TOKEN";

void
get_primary_ip(char *buffer,
               size_t buflen);

void
initialize_daemon();

void
validate_node(
    const int argc,
    const char *argv[]);

void
display_daemon_info();

void
start_websocket_server();

void
start_node();

int
main(const int argc,
     const char *argv[])
{
    initialize_daemon();
    validate_node(argc, argv);
    display_daemon_info();
    start_websocket_server();
    start_node();
    return 0;
}

void
get_primary_ip(char *buffer,
               size_t buflen)
{
    assert(buflen >= 16);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    assert(sock != -1);
    const char *kGoogleDnsIp = "8.8.8.8";

    uint16_t kDnsPort = 53;

    struct sockaddr_in serv;

    memset(&serv, 0, sizeof(serv));

    serv.sin_family = AF_INET;

    serv.sin_addr
        .s_addr = inet_addr(kGoogleDnsIp);

    serv.sin_port = htons(kDnsPort);
    int err = connect(sock, (const sockaddr *) &serv, sizeof(serv));
    assert(err != -1);
    sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (sockaddr *) &name, &namelen);
    assert(err != -1);
    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);
    assert(p);
    close(sock);
}


void load_peers()
{
    vector<string> peers;
    boost::filesystem::ifstream file("./peers");
    for (string line ; getline(file,line);)
        {
        boost::trim(line);
        if(';' != line.at(0))
            {
            using namespace boost::algorithm;
            std::vector<std::string> values;
            split(values, line, is_any_of("=:"));
            std::string peer = (values.size()==2 ? values[0] : values[1]); // 192.168.0.22:62000
            std::string port = (values.size()==2 ? values[1] : values[2]); // node_2=192.168.0.12:62001
            }
        }


}

// TODO: Move initialize_daemon out of main
void
initialize_daemon()
{
    // TODO: Mehdi, put your config file work here.
    // If we don't have a node id in the config file, then generate one here
    // Mehdi, for now we just generate a new  node id everytime we restart the node,
    // but we should really check the config file first.
    boost::uuids::basic_random_generator<boost::mt19937> gen;
    const boost::uuids::uuid node_id{gen()};
    DaemonInfo::get_instance().id() = node_id;

    char buffer[1024]{0};
    get_primary_ip(buffer, sizeof(buffer));
    DaemonInfo::get_instance().host_ip() = std::string(buffer);

    // load peers

}


// TODO: Move check_token_balance out of main
int
check_token_balance()
{
    auto token = getenv(s_etherscan_api_token_envar_name);
    if (token == nullptr)
        {
        std::cout << "Environment variable " << s_etherscan_api_token_envar_name
                  << " containing etherscan.io API token must be set" << std::endl;
        return 1;
        }

    DaemonInfo &daemon_info = DaemonInfo::get_instance();

    uint64_t balance = get_token_balance(daemon_info.ethereum_address(), token);
    if (balance < s_uint_minimum_required_token_balance)
        {
        std::cout << "Insufficient BZN token balance. The balance of "
                  << s_uint_minimum_required_token_balance << "BZN required to run node. Your balance is "
                  << balance << "BZN" << std::endl;
        return 1;
        }
    daemon_info.ropsten_token_balance() = balance;
    return 0;
}

// TODO: Move validate_node out of main and refactor
void
validate_node(
    const int argc,
    const char *argv[])
{
    if (0 != parse_command_line(argc, argv))
        {
        exit(-1);
        }

    if (0 != check_token_balance())
        {
        exit(-1);
        }
}

// TODO: Move display_daemon_info out of main into Daemon
void
display_daemon_info()
{
    DaemonInfo &daemon_info = DaemonInfo::get_instance();

    std::cout << "Running node with ID: " << daemon_info.id() << "\n"
              << " Ethereum Address ID: " << daemon_info.ethereum_address() << "\n"
              << "    Local IP Address: " << DaemonInfo::get_instance().host_ip() << "\n"
              << "             on port: " << daemon_info.host_port() << "\n"
              << "       Token Balance: " << daemon_info.ropsten_token_balance() << " BLZ\n"
              << std::endl;
}

// TODO: Move start_websocket_server out of main into Daemon
void
start_websocket_server()
{
    std::shared_ptr<Listener> listener;
    boost::thread websocket_thread
        (
            WebSocketServer
                (
                    DaemonInfo::get_instance().host_ip().c_str(),
                    DaemonInfo::get_instance().host_port() + 1000,
                    listener,
                    1
                )
        );
}

// TODO: Move start_node out of main into Daemon
void
start_node()
{
    boost::asio::io_service ios; // I/O service to use.
    boost::thread_group tg;
    Node this_node(ios);
    try
        {
        this_node.run();
        for (unsigned i = 0; i < boost::thread::hardware_concurrency(); ++i)
            {
            tg.create_thread(boost::bind(&boost::asio::io_service::run, &ios));
            }
        ios.run();
        }
    catch (std::exception &ex)
        {
        std::cout << ex.what() << std::endl;
        exit(1);
        }
}
