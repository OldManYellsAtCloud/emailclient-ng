#ifndef CURLREQUESTSCHEDULER_H
#define CURLREQUESTSCHEDULER_H

#include <atomic>
#include <functional>
#include <deque>
#include <thread>
#include "imap/curlrequest.h"

#include <QObject>

enum ImapRequestType {
    NOOP = 0, CAPABILITY, ENABLE, EXAMINE, LIST, FETCH,
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

    // comparison operator to be able to compare priority when pushing
    // tasks into priority queue - e.g. fetching mail has lower priority than
    // fetching missing UIDs.
    constexpr auto operator<=>(const ImapCurlRequest& other) const{
        return other.requestType <=> requestType;
    }

};

class CurlRequestScheduler: public QObject
{
    Q_OBJECT
private:
    CurlRequest *cr;
    std::thread taskThread;
    std::atomic_bool engineRunning = false;
    std::deque<ImapCurlRequest> taskQueue;
    //std::priority_queue<ImapCurlRequest> taskQueue;

    int delayMs;

    void executeRequests();
    void startExecutingThread();

public:
    CurlRequestScheduler(CurlRequest* imapRequest);

    template<typename... T>
    ImapCurlRequest createTask(ImapRequestType requestType, std::function<void(ResponseContent, std::string)> callback,
                               const std::string& cookie, T&&... params){
        ImapCurlRequest task = {};

        int stringCounter = 0;

        ([&]{
            switch(++stringCounter){
            case 1:
                task.param_s1 = params;
                break;
            case 2:
                task.param_s2 = params;
                break;
            case 3:
                task.param_s3 = params;
            }
        }(), ...);

        task.requestType = requestType;
        task.callback = callback;
        task.cookie = cookie;

        return std::move(task);
    }

    template<typename... T>
    ImapCurlRequest createTask(ImapRequestType requestType, std::function<void(ResponseContent, std::string)> callback,
                               const std::string& cookie, uint32_t param_i, T&&... params){
        ImapCurlRequest task = createTask(requestType, callback, cookie, params...);
        task.param_i = param_i;
        return std::move(task);
    }

    void addTask(ImapCurlRequest request);

signals:
    void fetchStarted();
    void fetchFinished();
};

#endif // CURLREQUESTSCHEDULER_H
