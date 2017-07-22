#ifndef KEPLER_BZDATASOURCE_H
#define KEPLER_BZDATASOURCE_H

#include <string>
#include <vector>

class BZDataSource {
public:
    virtual std::vector<std::string> GetData(){return std::vector<std::string> {};};
};
#endif //KEPLER_BZDATASOURCE_H
