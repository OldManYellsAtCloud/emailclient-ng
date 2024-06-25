#ifndef DBMANAGER_H
#define DBMANAGER_H

#include "mail.h"
#include "mailsettings.h"

#include <sqlite3.h>
#include <memory>
#include <functional>

class DbManager
{
private:
    const std::string CREATE_MAIL_TABLE = "CREATE TABLE IF NOT EXISTS mails "
                                          "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                          "uid INTEGER, "
                                          "folder TEXT, "
                                          "subject TEXT, "
                                          "sender TEXT, "
                                          "date TEXT, "
                                          "read BOOLEAN)";

    const std::string CREATE_MAILPARTS_TABLE = "CREATE TABLE IF NOT EXISTS mailparts "
                                               "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                               "mail_id INTEGER, "
                                               "type INTEGER, "
                                               "name TEXT, "
                                               "encoding INTEGER, "
                                               "content TEXT, "
                                               "FOREIGN KEY(mail_id) REFERENCES mails(id))";

    const std::string CREATE_INDEX = "CREATE UNIQUE INDEX IF NOT EXISTS mails_idx on mails(uid, folder)";

    const std::string CREATE_SETTINGS_TABLE = "CREATE TABLE IF NOT EXISTS settings "
                                              "(key TEXT PRIMARY KEY, value TEXT)";

    const std::string CREATE_FOLDERS_TABLE = "CREATE TABLE IF NOT EXISTS folders "
                                             "(original_name TEXT, readable_name TEXT, "
                                             "UNIQUE(original_name, readable_name))";

    const std::string INSERT_FOLDER = "INSERT INTO folders(original_name, readable_name) "
                                      "VALUES(:original_name, :readable_name)";
    const std::string SELECT_FOLDERS = "SELECT original_name, readable_name from folders";

    const std::string INSERT_MAIL = "INSERT INTO mails(uid, folder, subject, sender, date, read) "
                                    "VALUES(:uid, :folder, :subject, :sender, :date, :read)";
    const std::string GET_MAIL_DBID = "SELECT id FROM mails WHERE uid = :uid AND folder = :folder";
    const std::string INSERT_MAILPART = "INSERT INTO mailparts(mail_id, type, name, encoding, content) "
                                        "VALUES(:mail_id, :type, :name, :encoding, :content)";

    const std::string GET_EMAIL = "SELECT uid, folder, subject, sender, date, read FROM "
                                  "mails WHERE folder = :folder AND uid = :uid";
    const std::string GET_EMAIL_PARTS = "SELECT mail_id, type, name, encoding, content FROM "
                                        "mailparts WHERE mail_id = :mail_id";

    const std::string GET_ALL_UIDS_FROM_FOLDER = "SELECT uid FROM "
                                                 "mails WHERE folder = :folder ORDER BY uid DESC";

    const std::string MAIL_CACHED = "SELECT COUNT(*) FROM mails WHERE folder = :folder and uid = :uid";
    const std::string LAST_UID_FROM_FOLDER = "SELECT COALESCE(MAX(uid), -1) FROM mails WHERE folder = :folder";

    const std::string BEGIN_TRANSACTION = "BEGIN TRANSACTION;";
    const std::string END_TRANSACTION = "END TRANSACTION;";
    const std::string ROLLBACK_TRANSACTION = "ROLLBACK TRANSACTION;";


    std::unique_ptr<MailSettings> mailSettings;
    sqlite3* dbConnection;
    sqlite3_stmt* insert_mail_statement;
    sqlite3_stmt* insert_mailpart_statement;
    sqlite3_stmt* get_mail_dbid_statement;
    sqlite3_stmt* is_mail_cached_statement;
    sqlite3_stmt* get_last_cached_uid_statement;
    sqlite3_stmt* get_mail_statement;
    sqlite3_stmt* get_mailpart_statement;
    sqlite3_stmt* insert_folder_statement;
    sqlite3_stmt* select_folders_statement;
    sqlite3_stmt* get_all_uids_from_folder_statement;

    void initializeConnection();
    void initializeTable(const std::string& statement);
    void initializeTables();
    void prepareStatements();

    void storeMailInfo(const struct Mail& mail);
    Mail getMailInfo(const std::string& folder, const uint32_t uid);
    std::vector<MailPart> getMailParts(const int dbid);
    void storeMailParts(const struct Mail& mail);
    int getDbId(const struct Mail& mail);

    void resetStatementAndClearBindings(sqlite3_stmt* statement);

    void checkSuccess(int result, int expected_result, std::string info);

    int getParameterIndex(sqlite3_stmt* stmt, std::string parameter_name);

    std::vector<std::function<void(void)>> mailCallbacks;
    std::vector<std::function<void(void)>> folderCallbacks;

    std::vector<int> getAllUidsFromFolder(std::string folder);

    DbManager();
public:
    static DbManager* getInstance();
    ~DbManager();
    void storeEmail(const struct Mail& mail);
    bool isMailCached(int uid, std::string folder);

    void storeFolder(const std::string& original_name, const std::string& readable_name);
    std::vector<std::pair<std::string, std::string>> getFolderNames();
    bool areFoldersCached();

    int getLastCachedUid(std::string folder);
    Mail fetchMail(std::string folder, int uid);
    std::vector<Mail> getAllMailsFromFolder(std::string folder);

    void registerMailCallback(const std::function<void(void)> cb);
    void registerFolderCallback(const std::function<void(void)> cb);
};

#endif // DBMANAGER_H
