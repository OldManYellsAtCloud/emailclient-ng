#include "periodicdatafetcher.h"
#include "mailsettings.h"

PeriodicDataFetcher::PeriodicDataFetcher():
    curlRequestScheduler(&imapRequest),
    imapFetcher(&curlRequestScheduler, DbManager::getInstance()) {

    MailSettings ms{};
    refreshSeconds = ms.getRefreshFrequencySeconds();

    watchedFolders = ms.getWatchedFolders();

    fetchFolders();
    emailFetcherThread = std::jthread(&PeriodicDataFetcher::runEmailFetcherThread, this);
}

PeriodicDataFetcher::~PeriodicDataFetcher()
{
    emailFetcherThread.request_stop();
    emailFetcherThread.join();
}

void PeriodicDataFetcher::fetchFolder(const QString& folder)
{
    imapFetcher.fetchNewEmails(folder.toStdString());
}

void PeriodicDataFetcher::runEmailFetcherThread(std::stop_token stoken)
{
    while (!stoken.stop_requested()){
        for (const std::string folder: watchedFolders){
            imapFetcher.fetchNewEmails(folder);
        }
        std::this_thread::sleep_for(std::chrono::seconds(refreshSeconds));
    }
}

void PeriodicDataFetcher::fetchFolders(bool force)
{
    imapFetcher.fetchFoldersIfNeeded();
}

