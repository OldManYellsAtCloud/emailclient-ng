#include "imap/imapfetcher.h"
#include <loglibrary.h>
#include "utils.h"

ImapFetcher::ImapFetcher(CurlRequestScheduler *crs, DbManager* dm)
{
    dbManager = dm;
    curlRequestScheduler = crs;

    MailSettings ms{};
    daysToFetch = ms.getDaysToFetch();
}

void ImapFetcher::registerMailCallback(std::function<void ()> cb)
{
    mailCallbacks.push_back(cb);
}



void ImapFetcher::getLastUid(std::string folder)
{
    auto callback = [&](ResponseContent rc, std::string dummy){
        this->lastUidFetched(rc);
    };
    curlRequestScheduler->addTask(ImapRequestType::UID_SEARCH, folder, "MAX", "ALL", callback);
}

void ImapFetcher::lastUidFetched(ResponseContent rc)
{
    LOG("UID response arrived: {}", parseUid(rc.header.getResponse()));
}

void ImapFetcher::fetchNewEmails(std::string folder)
{
    int firstUid = dbManager->getLastCachedUid(folder);
    if (firstUid <= 0 ){
        std::string startingDate = getImapDateStringFromNDaysAgo(daysToFetch);
        auto callback = [&](ResponseContent rc, std::string dummy){
            this->receiveMinUidAndFetchMissingUids(rc);
        };
        curlRequestScheduler->addTask(ImapRequestType::UID_SEARCH, folder, "MIN", std::format("SINCE {}", startingDate), callback);
    } else {
        fetchMissingUids(folder, ++firstUid);
    }
}

void ImapFetcher::fetchFoldersIfNeeded()
{
    if (dbManager->areFoldersCached())
        return;

    auto callback = [&](ResponseContent rc, std::string dummy){
        this->folderListFetched(rc);
    };

    curlRequestScheduler->addTask(ImapRequestType::LIST, "\"\"", "*", callback, "");
}

uint32_t ImapFetcher::parseUid(const std::string &response)
{
    const std::string SEARCH_RESULT = "ESEARCH (TAG";
    std::vector<std::string> splitResponse = splitString(response, CRLF);
    int uid = -1;

    for (const std::string& s: splitResponse){
        if (s.find(SEARCH_RESULT) == std::string::npos)
            continue;

        std::vector<std::string> splitLine = splitString(s, SPACE);
        try {
            uid = std::stoi(splitLine[splitLine.size() - 1]);
            break;
        } catch (std::exception e){
            ERROR("Could not parse uid: {} : {}", s, e.what());
            break;
        }
    }
    return uid;
}

std::string ImapFetcher::parseFolder(const std::string &response)
{
    const std::string FOLDER_SEARCH_PATTERN = "selected. (Success)";
    std::vector<std::string> splitResponse = splitString(response, CRLF);
    std::string ret = "";
    for (const std::string& s: splitResponse){
        if (s.find(FOLDER_SEARCH_PATTERN) == std::string::npos)
            continue;

        std::vector<std::string> splitLine = splitString(s, SPACE);
        ret = splitLine[3];
        break;
    }
    return ret;
}

void ImapFetcher::receiveMinUidAndFetchMissingUids(ResponseContent rc)
{
    uint32_t minUid = parseUid(rc.header.getResponse());
    std::string folder = parseFolder(rc.header.getResponse());
    fetchMissingUids(folder, minUid);
}

void ImapFetcher::fetchMissingUids(std::string folder, uint32_t minUid)
{
    auto callback = [&](ResponseContent rc, std::string folder){
        this->receiveMissingUidsAndFetchMails(rc, folder);
    };
    curlRequestScheduler->addTask(ImapRequestType::UID_FETCH, folder, std::format("{}:*", minUid), "UID", callback, folder);
}

void ImapFetcher::receiveMissingUidsAndFetchMails(ResponseContent rc, std::string folder)
{
    std::vector<int> uids = parseUids(rc.header.getResponse());
    LOG("UID response: {}", rc.header.getResponse());
    filterOutOldUids(uids, folder);
    fetchMissingEmailsByUid(uids, folder);
}

std::vector<int> ImapFetcher::parseUids(const std::string &response)
{
    const std::string UID_START_PATTERN = "(UID ";
    const char UID_END_PATTERN = ')';

    std::vector<std::string> splitResponse = splitString(response, CRLF);
    std::vector<int> uids;
    int i;
    for (const std::string& s: splitResponse){
        size_t start = s.find(UID_START_PATTERN);
        if (start == std::string::npos)
            continue;

        start += UID_START_PATTERN.size();
        size_t end = s.find(UID_END_PATTERN, start);
        try {
            i = std::stoi(s.substr(start, end - start));
            uids.push_back(i);
        } catch (std::exception e){
            ERROR("Could not convert to number: {}", s.substr(start, end - start));
        }
    }

    return uids;
}

void ImapFetcher::fetchMissingEmailsByUid(const std::vector<int> &uids, std::string folder)
{
    auto callback = [&](ResponseContent rc, std::string folder){
        this->parseAndStoreEmail(rc, folder);
    };

    for (const int& uid: uids) {
        curlRequestScheduler->addTask(ImapRequestType::UID_FETCH, folder, std::to_string(uid),
                                      "BODY[]", callback, folder);
    }
}

void ImapFetcher::parseAndStoreEmail(ResponseContent rc, std::string folder)
{
    Mail mail = imapMailParser.parseImapResponseToMail(rc, folder);
    dbManager->storeEmail(mail);

    for (const auto& callback: mailCallbacks)
        callback();
}

/**
 * @brief ImapFetcher::filterOutOldUids
 * @param uids
 * @param folder
 *
 * It checks the last cached UID, and removes all elements from the
 * uid list which are equal or smaller than that.
 *
 * One would expect that this is handled on IMAP side, however when
 * new UIDs are queried, even if the lower limit is bigger than the
 * existing one, the last existing one is still returned, instead of
 * an empty list.
 */
void ImapFetcher::filterOutOldUids(std::vector<int> &uids, std::string folder)
{
    int minUid = dbManager->getLastCachedUid(folder);
    auto newVectorEnd = std::remove_if(uids.begin(), uids.end(), [&](const int& i){return i <= minUid;});
    uids.resize(newVectorEnd - uids.begin());
}

/**
 * @brief ImapFetcher::folderListFetched
 * @param rc
 * Used as a callback after folder list is fetched.
 * This function parses the response and stores the folder names in the database.
 */
void ImapFetcher::folderListFetched(ResponseContent rc)
{
    std::vector<std::string> folders = parseFolderResponse(rc.body.getResponse());
    for (const std::string& folderName: folders){
        // TODO: decode foldername, and store the decoded version alongside the original
        dbManager->storeFolder(folderName, folderName);
    }
}

/**
 * @brief ImapFetcher::parseFolderResponse
 * @param response
 * @return List of parsed folder names, in original, possibly encoded form.
 */
std::vector<std::string> ImapFetcher::parseFolderResponse(const std::string &response)
{
    std::vector<std::string> parsed_folders;
    std::vector<std::string> ret = splitString(response, CRLF);

    for (std::string s: ret){
        if (!isStringEmpty(s)){
            std::vector<std::string> v = quoteAwareSplitString(s, " ");
            std::string decodedFolder = unquoteString(v[v.size() - 1]);
            //decodedFolder = decodeImapUTF7(decodedFolder);
            parsed_folders.emplace_back(decodedFolder);
        }
    }

    return parsed_folders;
}

