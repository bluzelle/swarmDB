#include "Utility.h"

#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>

std::string getStringFromThreadId(const std::thread::id &idThread)
{
    std::stringstream ss;
    ss << idThread;
    return ss.str();
}




std::string getCPPVersionString()
{
    if ( __cplusplus == 201402L )
        {
        return "C++14" ;
        }

    else if ( __cplusplus == 201103L )
        {
        return "C++11" ;
        }

    else if( __cplusplus == 19971L )
        {
        return "C++98" ;
        }

    else
        {
        return "pre-standard C++" ;
        }
}


void printCPPVersion()
{
    std::cout << getCPPVersionString() << std::endl;
}


std::string getBuildInformationString()
{
    const std::string strRetVal = getCPPVersionString() + " - " + __DATE__ + " - " + __TIME__;

    std::cout << strRetVal;

    return strRetVal;
}

