#include "imap/curlresponse.h"

curlResponse::curlResponse()
{
    r = "";
    length = r.length();
}

curlResponse::~curlResponse()
{

}

size_t curlResponse::storeResponse(char *ptr, size_t nmemb)
{
    std::string newContent(ptr, nmemb);
    r.append(newContent);
    length += nmemb;
    return nmemb;
}

std::string curlResponse::getResponse()
{
    return r;
}

size_t curlResponse::getResponseSize()
{
    return length;
}

bool curlResponse::success()
{
    return success_;
}

void curlResponse::setSuccess(bool success)
{
    success_ = success;
}

void curlResponse::reset()
{
    r = "";
    length = r.length();
    success_ = true;
}
