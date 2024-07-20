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
    bool isFetchInProgress;

    Q_PROPERTY(bool fetchInProgress READ getFetchInProgress NOTIFY fetchInProgressChanged FINAL)

    void runEmailFetcherThread(std::stop_token stoken);
    void fetchFolders(bool force = false);

    void notifyNewEmail();
    void notifyNewFolders();
public:
    PeriodicDataFetcher();
    ~PeriodicDataFetcher();
    void fetchFolder(const std::string& folder);
    Q_INVOKABLE void fetchFolder(const int& index);
    bool getFetchInProgress();

public slots:
    void fetchStarted();
    void fetchFinished();

signals:
    void fetchInProgressChanged();
};

#endif // PERIODICDATAFETCHER_H
