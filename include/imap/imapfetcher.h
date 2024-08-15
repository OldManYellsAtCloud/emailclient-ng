#ifndef IMAPFETCHER_H
#define IMAPFETCHER_H

#include "dbmanager.h"
#include "imapmailparser.h"
#include "curlrequestscheduler.h"

class ImapFetcher
{
    DbManager* dbManager;
    CurlRequestScheduler* curlRequestScheduler;
    ImapMailParser imapMailParser;

    int parseUid(const std::string& response);
    std::string parseFolder(const std::string& response);

    int daysToFetch;
    void receiveMinUidAndFetchMissingUids(ResponseContent rc);
    void fetchMissingUids(std::string folder, int minUid);
    void receiveMissingUidsAndFetchMails(ResponseContent rc, std::string folder);
    std::vector<int> parseUids(const std::string& response);
    void fetchMissingEmailsByUid(const std::vector<int>& uids, std::string folder);
    void parseAndStoreEmail(ResponseContent rc, std::string folder);

    void filterOutOldUids(std::vector<int>& uids, std::string folder);

    void folderListFetched(ResponseContent rc);
    std::vector<std::string> parseFolderResponse(const std::string& response);

    std::vector<std::function<void(void)>> mailCallbacks;

public:
    ImapFetcher(CurlRequestScheduler* crs, DbManager* dm);
    void registerMailCallback(std::function<void(void)> cb);
    void getLastUid(std::string folder);

    void lastUidFetched(ResponseContent rc);
    void fetchNewEmails(std::string folder);
    void fetchFoldersIfNeeded();
};

#endif // IMAPFETCHER_H
