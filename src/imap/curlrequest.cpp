#include "imap/curlrequest.h"
#include <loglibrary.h>
#include <thread>

#include <QUrl> // for percent encoding

std::string constructUrl(std::string host, int port){
    auto prefix = port == IMAPS_PORT ? "imaps://" : "imap://";
    return std::format("{}{}:{}", prefix, host, port);
}

void CurlRequest::waitForRateLimit()
{
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    auto diffSinceLastRequest = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRequestTime);

    if (diffSinceLastRequest.count() < requestDelayMs) {
        auto sleepBeforeNextRequest = requestDelayMs - diffSinceLastRequest.count();
        DBG("Rate limit, sleep {}ms", sleepBeforeNextRequest);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepBeforeNextRequest));
    }
}

CurlRequest::CurlRequest()
{
    mailSettings = std::make_unique<MailSettings>();
    requestDelayMs = mailSettings->getImapRequestDelay();
    lastRequestTime = std::chrono::steady_clock::now();
    initializeCurl();
}

CurlRequest::~CurlRequest()
{
    curl_easy_cleanup(curl);
}

static size_t storeCurlData(char *ptr, size_t size, size_t nmemb, void *userdata){
    curlResponse *cr = (curlResponse*)userdata;
    std::string s(ptr, nmemb);
    return cr->storeResponse(ptr, nmemb);
}

void CurlRequest::prepareCurlRequest(const std::string& url, const std::string& customRequest)
{
    header.reset();
    body.reset();
    LOG("Preparing request with url: {}, command: {}", url, customRequest);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, storeCurlData);

    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, storeCurlData);

    if (customRequest.empty())
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
    else
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, customRequest.c_str());
}

void CurlRequest::initializeCurl()
{
    curl = curl_easy_init();

    auto imapAddress = mailSettings->getImapServerAddress();
    int imapPort = mailSettings->getImapServerPort();

    serverAddress = constructUrl(imapAddress, imapPort);

    auto userName = mailSettings->getUserName();
    auto password = mailSettings->getPassword();

    curl_easy_setopt(curl, CURLOPT_USERNAME, userName.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
}

CURLcode CurlRequest::performCurlRequest()
{

    waitForRateLimit();
    lastRequestTime = std::chrono::steady_clock::now();

    auto res = curl_easy_perform(curl);
    DBG("Body response: {}", body.getResponse().substr(0, 1000));
    DBG("Header response: {}", header.getResponse().substr(0, 1000));

    return res;
}

ResponseContent CurlRequest::NOOP()
{
    prepareCurlRequest(serverAddress, "NOOP");
    performCurlRequest();
    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}

ResponseContent CurlRequest::CAPABILITY()
{
    prepareCurlRequest(serverAddress, "CAPABILITY");
    performCurlRequest();

    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}

ResponseContent CurlRequest::ENABLE(std::string capability)
{
    std::string cmd = std::format("ENABLE {}", capability);
    prepareCurlRequest(serverAddress, cmd);
    performCurlRequest();
    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}

ResponseContent CurlRequest::EXAMINE(std::string folder)
{
    folder = QUrl::toPercentEncoding(QString::fromStdString(folder)).toStdString();
    std::string cmd = std::format("EXAMINE {}", folder);
    prepareCurlRequest(serverAddress, cmd);
    performCurlRequest();
    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}

ResponseContent CurlRequest::LIST(std::string reference, std::string mailbox)
{
    mailbox = QUrl::toPercentEncoding(QString::fromStdString(mailbox)).toStdString();
    std::string cmd = std::format("LIST {} {}", reference, mailbox);
    prepareCurlRequest(serverAddress, cmd);
    performCurlRequest();
    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}

ResponseContent CurlRequest::FETCH(std::string folder, uint32_t messageindex, std::string item)
{
    folder = QUrl::toPercentEncoding(QString::fromStdString(folder)).toStdString();
    std::string addr = std::format("{}/{}", serverAddress, folder);
    std::string cmd = std::format("FETCH {} {}", messageindex, item);
    prepareCurlRequest(addr, cmd);
    performCurlRequest();
    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}

ResponseContent CurlRequest::FETCH_MULTI_MESSAGE(std::string folder, std::string indexRange, std::string item)
{
    folder = QUrl::toPercentEncoding(QString::fromStdString(folder)).toStdString();
    std::string cmd = std::format("FETCH {} {}", indexRange, item);
    prepareCurlRequest(serverAddress, cmd);
    performCurlRequest();
    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}

ResponseContent CurlRequest::UID_FETCH(std::string folder, std::string uid, std::string item)
{
    folder = QUrl::toPercentEncoding(QString::fromStdString(folder)).toStdString();
    std::string addr = std::format("{}/{}", serverAddress, folder);
    std::string cmd = std::format("UID FETCH {} ({})", uid, item);
    prepareCurlRequest(addr, cmd);
    performCurlRequest();
    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}

ResponseContent CurlRequest::UID_SEARCH(std::string folder, std::string item_to_return, std::string criteria)
{
    folder = QUrl::toPercentEncoding(QString::fromStdString(folder)).toStdString();
    std::string addr = std::format("{}/{}", serverAddress, folder);
    std::string cmd = std::format("UID SEARCH RETURN ({}) {}", item_to_return, criteria);
    prepareCurlRequest(addr, cmd);
    performCurlRequest();
    struct ResponseContent rp;
    rp.body = body;
    rp.header = header;
    return rp;
}
