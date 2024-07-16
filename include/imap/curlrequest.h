#ifndef CURLREQUEST_H
#define CURLREQUEST_H

#include <string>
#include <chrono>

#include <stdint.h>
#include <curl/curl.h>

#include "imaprequestinterface.h"
#include "mailsettings.h"

#define IMAPS_PORT 993


class CurlRequest: public ImapRequestInterface {
private:
    std::string serverAddress;

    int requestDelayMs = 0;
    std::chrono::time_point<std::chrono::steady_clock> lastRequestTime;

    CURL *curl;
    curlResponse body, header;

    void initializeCurl();
    CURLcode performCurlRequest();
    void prepareCurlRequest(const std::string& url, const std::string& customRequest = "");

    void waitForRateLimit();
    void configureImap();


    std::unique_ptr<MailSettings> mailSettings;

public:
    CurlRequest();
    ~CurlRequest();

    ResponseContent NOOP() override;
    ResponseContent CAPABILITY() override;
    ResponseContent ENABLE(std::string capability) override;
    ResponseContent EXAMINE(std::string folder) override;
    ResponseContent LIST(std::string reference, std::string mailbox) override;
    ResponseContent FETCH(std::string folder, uint32_t messageindex, std::string item) override;
    ResponseContent FETCH_MULTI_MESSAGE(std::string folder, std::string indexRange, std::string item) override;
    ResponseContent UID_FETCH(std::string folder, std::string uid, std::string item) override;
    ResponseContent UID_SEARCH(std::string folder, std::string item_to_return, std::string criteria) override;
};

#endif // CURLREQUEST_H
