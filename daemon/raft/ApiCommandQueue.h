#ifndef BLUZELLE_APICOMMANDQUEUE_H
#define BLUZELLE_APICOMMANDQUEUE_H

#include <string>
#include <queue>
#include <utility>

using std::queue;
using std::pair;
using std::string;

class ApiCommandQueue : public queue<pair<string,string>>
{
public:

};

#endif //BLUZELLE_APICOMMANDQUEUE_H
