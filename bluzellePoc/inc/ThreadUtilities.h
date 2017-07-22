#ifndef KEPLER_THREADUTILITIES_H
#define KEPLER_THREADUTILITIES_H

#include <thread>
#include <string>

int getThreadFriendlyLargeRandomNumber();
void lockedCout(const std::string &str);
void threadFunction(const unsigned int i);

#endif //KEPLER_THREADUTILITIES_H
