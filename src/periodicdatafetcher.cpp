#include "periodicdatafetcher.h"
#include "mailsettings.h"

PeriodicDataFetcher::PeriodicDataFetcher():
    curlRequestScheduler(&curlRequest),
    imapFetcher(&curlRequestScheduler, DbManager::getInstance()) {

    MailSettings ms{};
    refreshSeconds = ms.getRefreshFrequencySeconds();

    watchedFolders = ms.getWatchedFolders();
    connect(&curlRequestScheduler, &CurlRequestScheduler::fetchStarted, this, &PeriodicDataFetcher::fetchStarted);
    connect(&curlRequestScheduler, &CurlRequestScheduler::fetchFinished, this, &PeriodicDataFetcher::fetchFinished);

    fetchFolders();
    emailFetcherThread = std::jthread(&PeriodicDataFetcher::runEmailFetcherThread, this);
}

PeriodicDataFetcher::~PeriodicDataFetcher()
{
    emailFetcherThread.request_stop();
    emailFetcherThread.join();
}

void PeriodicDataFetcher::fetchFolder(const std::string& folder)
{
    imapFetcher.fetchNewEmails(folder);
}

void PeriodicDataFetcher::fetchFolder(const int &index)
{
    std::string canonicalFolderName = DbManager::getInstance()->getCanonicalFolderName(index);
    fetchFolder(canonicalFolderName);
}

bool PeriodicDataFetcher::getFetchInProgress()
{
    return isFetchInProgress;
}

void PeriodicDataFetcher::fetchStarted()
{
    isFetchInProgress = true;
    emit fetchInProgressChanged();
}

void PeriodicDataFetcher::fetchFinished()
{
    isFetchInProgress = false;
    emit fetchInProgressChanged();
}

void PeriodicDataFetcher::runEmailFetcherThread(std::stop_token stoken)
{
    int counter = 0;
    while (!stoken.stop_requested()){
        if ((counter++) % refreshSeconds == 0 ) {
            for (const std::string folder: watchedFolders){
                imapFetcher.fetchNewEmails(folder);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void PeriodicDataFetcher::fetchFolders(bool force)
{
    imapFetcher.fetchFoldersIfNeeded();
}

