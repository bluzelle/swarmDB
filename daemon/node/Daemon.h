#ifndef BLUZELLE_DAEMON_H
#define BLUZELLE_DAEMON_H

#include "DaemonInfo.h"

#include <string>

using namespace std;

// TODO: This is unused right now, but will be used in main.

class Daemon
{
    DaemonInfo *daemon_info_;

public:
    Daemon();

    ~Daemon() = default;

    string node_id();

    string ethereum_address();

    uint16_t get_port();

    uint16_t get_token_balance();

    string get_info();
};

#endif //BLUZELLE_DAEMON_H
