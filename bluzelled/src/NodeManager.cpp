#include "NodeManager.h"

unsigned long number_of_nodes_to_create(unsigned long min_nodes, unsigned long max_nodes, unsigned current_number_of_nodes)
{
    return current_number_of_nodes < min_nodes ? min_nodes - current_number_of_nodes : (current_number_of_nodes < max_nodes ? 1 : 0);
}
