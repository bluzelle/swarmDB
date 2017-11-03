#include "Node.h"

#include <string>

// TODO: Consider enapsulating these functions into a Node Manager.
bool all_nodes_alive(const Nodes& nodes);
void wait_for_all_nodes_to_start(const Nodes &nodes);
void reaper(Nodes *nodes);
std::string generate_ip_address();
