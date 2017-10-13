#ifndef KEPLER_HTTPREQUEST_HPP_H
#define KEPLER_HTTPREQUEST_HPP_H

#include <string>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using std::string;


class HttpResponse {
protected:
    string text;

public:
    HttpResponse(const string& s) : text(s) {}

    unsigned int getStatus() {
        boost::regex regex("^HTTP\\/\\d\\.\\d (\\d{3}).+$"); // Test regex: https://regex101.com/
        boost::sregex_token_iterator it(text.begin(), text.end(), regex, 1/*subgroup*/);
        boost::sregex_token_iterator end;

        auto s = it->str();
        unsigned int status = boost::lexical_cast<unsigned int>(s);

        return status;
    }

    string getContent() {
        auto l = getContentLength();

        if (l > 0) {
            return text.substr(text.length() - l, l);
        }

        return string();
    }

    unsigned int getContentLength() {
        boost::regex regex("^Content-Length: (\\d+)$");
        boost::sregex_token_iterator it(text.begin(), text.end(), regex, 1/*subgroup*/);
        boost::sregex_token_iterator end;

        auto s = it->str();
        unsigned int length = boost::lexical_cast<unsigned int>(s);

        return length;
    }
};

#endif //KEPLER_HTTPREQUEST_HPP_H
