#include "curlrequestscheduler.h"
#include <loglibrary.h>

void CurlRequestScheduler::executeRequests()
{
    emit fetchStarted();
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
        taskQueue.pop_front();
    }
    emit fetchFinished();
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

void CurlRequestScheduler::addTask(ImapCurlRequest request)
{
    taskQueue.push_back(request);
    startExecutingThread();
}

