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
    CurlRequest curlRequest;
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
    Q_INVOKABLE void fetchFolder(const QString& folder);
};

#endif // PERIODICDATAFETCHER_H
