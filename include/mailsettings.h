#ifndef MAILSETTINGS_H
#define MAILSETTINGS_H

#include <settingslib.h>

class MailSettings
{
private:
    SettingsLib settings;
public:
    MailSettings();
    std::string getImapServerAddress();
    int getImapServerPort();
    std::string getUserName();
    std::string getPassword();
    std::string getDbPath();
    std::string getApplicationUser();
    int getDaysToFetch();
    std::vector<std::string> getWatchedFolders();
    int getImapRequestDelay();
    int getRefreshFrequencySeconds();
};

#endif // MAILSETTINGS_H
