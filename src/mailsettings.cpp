#include "mailsettings.h"
#include "utils.h"
#include <loglibrary.h>


#define DEFAULT_IMAP_PORT  993
#define DEFAULT_MAIL_DAYS_TO_FETCH  10
#define DEFAULT_MAIL_REFRESH_FREQ_SECONDS 900

MailSettings::MailSettings(): settings{"/etc"}
{
}

std::string MailSettings::getImapServerAddress()
{
    return settings.getValue("mail", "imapServerAddress");
}

int MailSettings::getImapServerPort()
{
    try {
        return std::stoi(settings.getValue("mail", "imapServerPort"));
    } catch (std::exception e) {
        ERROR("Could not get imap server port: {}", e.what());
        return DEFAULT_IMAP_PORT;
    }
}

std::string MailSettings::getUserName()
{
    return settings.getValue("mail", "userName");
}

std::string MailSettings::getApplicationUser()
{
    return settings.getValue("general", "applicationUser");
}

std::string MailSettings::getPassword()
{
    auto passwordBase64 = settings.getValue("mail", "password");
    std::string password = decodeBase64String(passwordBase64);
    return password;
}

std::string MailSettings::getDbPath()
{
    return settings.getValue("mail", "dbPath");
}

int MailSettings::getDaysToFetch()
{
    try {
        return std::stoi(settings.getValue("mail", "daysToFetch"));
    } catch (std::exception e) {
        ERROR("Could not get number of mails to fetch: {}", e.what());
        return DEFAULT_MAIL_DAYS_TO_FETCH;
    }
}

std::vector<std::string> MailSettings::getWatchedFolders()
{
    std::string folderString = settings.getValue("mail", "watchedFolders");
    std::vector<std::string> splitFolder = splitString(folderString, ",");
    return splitFolder;
}

int MailSettings::getImapRequestDelay()
{
    try {
        return std::stoi(settings.getValue("mail", "imapRequestDelay"));
    } catch (std::exception e) {
        ERROR("Could not get imapRequestDelay config: {}", e.what());
        return 0;
    }
}

int MailSettings::getRefreshFrequencySeconds()
{
    try {
        return std::stoi(settings.getValue("mail", "refreshFrequencySeconds"));
    } catch (std::exception e){
        ERROR("Could not get refreshFrequencySeconds: {}", e.what());
        return DEFAULT_MAIL_REFRESH_FREQ_SECONDS;
    }
}
