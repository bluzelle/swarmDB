#ifndef BLUZELLE_UTILITY_H
#define BLUZELLE_UTILITY_H

#include <string>
#include <thread>
#include <iostream>



std::string getStringFromThreadId(const std::thread::id &idThread);
std::string getBuildInformationString();
std::string getCPPVersionString();

void threadIntroduction(const unsigned int i);
void printCPPVersion();
void threadFunction(const unsigned int i);

#endif //BLUZELLE_UTILITY_H
