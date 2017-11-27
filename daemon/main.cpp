 // TODO: move this to the correct module

#include <iostream>
#include <boost/thread.hpp>


int main() {
    try
        {
        std::cout << "I refuse to do Hello, World!" << std::endl;
        }
    catch
            (
            const std::exception &e
            )
        {
        std::cerr << "std::exception: [" << e.what() << "]" << std::endl;
        }


    return 0;
}