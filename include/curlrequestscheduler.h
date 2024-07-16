#ifndef CURLREQUESTSCHEDULER_H
#define CURLREQUESTSCHEDULER_H

#include <atomic>
#include <functional>
#include <queue>
#include <thread>
#include "imap/curlrequest.h"

#include <QObject>

enum class ImapRequestType {
    NOOP, CAPABILITY, ENABLE, EXAMINE, LIST, FETCH,
    FETCH_MULTI_MESSAGE, UID_FETCH, UID_SEARCH
};

struct ImapCurlRequest {
    ImapRequestType requestType;
    std::string param_s1;
    std::string param_s2;
    std::string param_s3;
    uint32_t param_i;
    std::function<void(ResponseContent, std::string)> callback;
    std::string cookie;
};

class CurlRequestScheduler: public QObject
{
    Q_OBJECT
private:
    CurlRequest *cr;
    std::thread taskThread;
    std::atomic_bool engineRunning = false;
    std::queue<ImapCurlRequest> taskQueue;

    int delayMs;

    void executeRequests();
    void startExecutingThread();

public:
    CurlRequestScheduler(CurlRequest* imapRequest);
    void addTask(ImapRequestType requestType, std::function<void(ResponseContent, std::string)> callback, const std::string& cookie = "");
    void addTask(ImapRequestType requestType, std::string param_s, std::function<void(ResponseContent, std::string)> callback, const std::string& cookie = "");
    void addTask(ImapRequestType requestType, uint32_t param_i, std::string param_s1, std::string param_s2, std::function<void(ResponseContent, std::string)> callback, const std::string& cookie = "");
    void addTask(ImapRequestType requestType, std::string param_s1, std::string param_s2, std::function<void(ResponseContent, std::string)> callback, const std::string& cookie = "");
    void addTask(ImapRequestType requestType, std::string param_s1, std::string param_s2, std::string param_s3, std::function<void(ResponseContent, std::string)> callback, const std::string& cookie = "");

signals:
    void fetchStarted();
    void fetchFinished();
};

#endif // CURLREQUESTSCHEDULER_H
