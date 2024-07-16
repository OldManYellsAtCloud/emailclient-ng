#include "curlrequestscheduler.h"
#include <loglibrary.h>

void CurlRequestScheduler::executeRequests()
{
    ResponseContent rc;
    while (!taskQueue.empty()){
        LOG("Taskqueue size: {}", taskQueue.size());
        ImapCurlRequest request = taskQueue.front();
        switch (request.requestType){
        case ImapRequestType::NOOP:
            rc = cr->NOOP();
            break;
        case ImapRequestType::CAPABILITY:
            rc = cr->CAPABILITY();
            break;
        case ImapRequestType::ENABLE:
            rc = cr->ENABLE(request.param_s1);
            break;
        case ImapRequestType::EXAMINE:
            rc = cr->EXAMINE(request.param_s1);
            break;
        case ImapRequestType::LIST:
            rc = cr->LIST(request.param_s1, request.param_s2);
            break;
        case ImapRequestType::FETCH:
            rc = cr->FETCH(request.param_s1, request.param_i, request.param_s2);
            break;
        case ImapRequestType::UID_FETCH:
            rc = cr->UID_FETCH(request.param_s1, request.param_s2, request.param_s3);
            break;
        case ImapRequestType::FETCH_MULTI_MESSAGE:
            rc = cr->FETCH_MULTI_MESSAGE(request.param_s1, request.param_s2, request.param_s3);
            break;
        case ImapRequestType::UID_SEARCH:
            rc = cr->UID_SEARCH(request.param_s1, request.param_s2, request.param_s3);
            break;
        }

        request.callback(rc, request.cookie);
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs)); // rate limit
        taskQueue.pop();
    }
    engineRunning = false;
}

void CurlRequestScheduler::startExecutingThread()
{
    if (engineRunning)
        return;
    engineRunning = true;
    taskThread = std::thread(&CurlRequestScheduler::executeRequests, this);
    taskThread.detach();
}

CurlRequestScheduler::CurlRequestScheduler(CurlRequest *curlRequest) {
    cr = curlRequest;
    MailSettings ms {};
    delayMs = ms.getImapRequestDelay();
}

void CurlRequestScheduler::addTask(ImapRequestType requestType, std::function<void (ResponseContent, std::string)> callback, const std::string& cookie)
{
    if (requestType != ImapRequestType::NOOP && requestType != ImapRequestType::CAPABILITY){
        throw std::runtime_error("Incorrect requestType. Only NOOP and CAPABILITY take no argument");
    }

    ImapCurlRequest request = {
        .requestType = requestType,
        .callback = callback,
        .cookie = cookie
    };
    taskQueue.push(request);
    startExecutingThread();
}

void CurlRequestScheduler::addTask(ImapRequestType requestType, std::string param_s, std::function<void (ResponseContent, std::string)> callback, const std::string& cookie)
{
    if (requestType != ImapRequestType::ENABLE && requestType != ImapRequestType::EXAMINE){
        throw std::runtime_error("Incorrect requestType. Only ENABLE and EXAMINE take 1 string argument");
    }

    ImapCurlRequest request = {
        .requestType = requestType,
        .param_s1 = param_s,
        .callback = callback,
        .cookie = cookie
    };
    taskQueue.push(request);
    startExecutingThread();
}

void CurlRequestScheduler::addTask(ImapRequestType requestType, uint32_t param_i, std::string param_s1, std::string param_s2, std::function<void (ResponseContent, std::string)> callback, const std::string& cookie)
{
    if (requestType != ImapRequestType::FETCH && requestType != ImapRequestType::UID_FETCH){
        throw std::runtime_error("Incorrect requestType. Only FETCH takes 2 string and 1 int arguments.");
    }

    ImapCurlRequest request = {
        .requestType = requestType,
        .param_s1 = param_s1,
        .param_s2 = param_s2,
        .param_i = param_i,
        .callback = callback,
        .cookie = cookie
    };
    taskQueue.push(request);
    startExecutingThread();
}

void CurlRequestScheduler::addTask(ImapRequestType requestType, std::string param_s1, std::string param_s2, std::function<void (ResponseContent, std::string)> callback, const std::string& cookie)
{
    if (requestType != ImapRequestType::LIST){
        throw std::runtime_error("Incorrect requestType. Only LIST takes 2 string arguments");
    }

    ImapCurlRequest request = {
        .requestType = requestType,
        .param_s1 = param_s1,
        .param_s2 = param_s2,
        .callback = callback,
        .cookie = cookie
    };
    taskQueue.push(request);
    startExecutingThread();
}

void CurlRequestScheduler::addTask(ImapRequestType requestType, std::string param_s1, std::string param_s2, std::string param_s3, std::function<void(ResponseContent, std::string)> callback, const std::string& cookie)
{
    if (requestType != ImapRequestType::FETCH_MULTI_MESSAGE && requestType != ImapRequestType::UID_SEARCH
        && requestType != ImapRequestType::UID_FETCH){
        throw std::runtime_error("Incorrect requestType. Only FETCH_MULTI_MESSAGE, UID_SEARCH and UID_FETCH take 3 string arguments");
    }

    ImapCurlRequest request = {
        .requestType = requestType,
        .param_s1 = param_s1,
        .param_s2 = param_s2,
        .param_s3 = param_s3,
        .callback = callback,
        .cookie = cookie
    };
    taskQueue.push(request);
    startExecutingThread();
}


