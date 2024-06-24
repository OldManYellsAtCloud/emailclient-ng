#ifndef PERIODICDATAFETCHER_H
#define PERIODICDATAFETCHER_H

#include <QObject>
#include <QQmlEngine>
#include <thread>
#include "imap/imapfetcher.h"

class PeriodicDataFetcher: public QObject
{
    Q_OBJECT
    QML_ELEMENT
private:
    ImapCurlRequest imapCurlRequest;
    ImapFetcher imapFetcher;
    ImapRequest imapRequest;
    CurlRequestScheduler curlRequestScheduler;
    std::vector<std::string> watchedFolders;
    std::jthread emailFetcherThread;

    int refreshSeconds;

    void runEmailFetcherThread(std::stop_token stoken);
    void fetchFolders(bool force = false);

    void notifyNewEmail();
    void notifyNewFolders();
public:
    PeriodicDataFetcher();
    ~PeriodicDataFetcher();

signals:
    void mailArrived();
    void foldersFetched();
};

#endif // PERIODICDATAFETCHER_H
