#ifndef CURLRESPONSE_H
#define CURLRESPONSE_H

#include <stddef.h>
#include <string>

class curlResponse
{
    std::string r;
    bool success_;
    size_t length;
public:
    curlResponse();
    ~curlResponse();
    size_t storeResponse(char* ptr, size_t nmemb);
    std::string getResponse();
    size_t getResponseSize();
    bool success();
    void setSuccess(bool success);
    void reset();
};

#endif // CURLRESPONSE_H
