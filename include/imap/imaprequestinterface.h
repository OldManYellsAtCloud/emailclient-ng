#ifndef IMAPREQUESTINTERFACE_H
#define IMAPREQUESTINTERFACE_H

#include "curlresponse.h"
#include <cstdint>

struct ResponseContent {
    curlResponse body;
    curlResponse header;
};

class ImapRequestInterface {
public:
    virtual ~ImapRequestInterface() {};
    virtual ResponseContent NOOP() = 0;
    virtual ResponseContent CAPABILITY() = 0;
    virtual ResponseContent ENABLE(std::string capability) = 0;
    virtual ResponseContent EXAMINE(std::string folder) = 0;
    virtual ResponseContent LIST(std::string reference, std::string mailbox) = 0;
    virtual ResponseContent FETCH(std::string folder, uint32_t messageindex, std::string item) = 0;
    virtual ResponseContent FETCH_MULTI_MESSAGE(std::string folder, std::string indexRange, std::string item) = 0;
    virtual ResponseContent UID_FETCH(std::string folder, std::string uid, std::string item) = 0;
    virtual ResponseContent UID_SEARCH(std::string folder, std::string item_to_return, std::string criteria) = 0;
};

#endif // IMAPREQUESTINTERFACE_H
