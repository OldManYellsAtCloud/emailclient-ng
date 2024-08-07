#ifndef DBMANAGER_H
#define DBMANAGER_H

#include "mail.h"
#include "mailsettings.h"

#include <sqlite3.h>
#include <mutex>
#include <memory>
#include <functional>

#define LATEST_DB_VERSION 3

class DbManager
{
private:
    std::mutex dbLock;

    enum FolderNameType {
        CANONICAL, READABLE
    };

    const std::string GET_DB_VERSION = "SELECT CASE "
                                       "(SELECT COUNT(*) FROM settings WHERE key = 'DB_VERSION') "
                                       "WHEN 0 THEN '1' "
                                       "ELSE "
                                       "(SELECT value FROM settings WHERE key = 'DB_VERSION') "
                                       "END";

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
                                              "(key TEXT PRIMARY KEY, value TEXT);";

    const std::string CREATE_FOLDERS_TABLE = "CREATE TABLE IF NOT EXISTS folders "
                                             "(original_name TEXT, readable_name TEXT, "
                                             "UNIQUE(original_name, readable_name))";

    const std::string INSERT_FOLDER = "INSERT INTO folders(canonical_name, readable_name) "
                                      "VALUES(:canonical_name, :readable_name)";
    const std::string SELECT_FOLDERS = "SELECT original_name, readable_name from folders";

    const std::string INSERT_MAIL = "INSERT INTO mails(uid, folder, subject, sender_email, sender_name, date, read) "
                                    "VALUES(:uid, :folder, :subject, :sender_email, :sender_name, :date, :read)";
    const std::string GET_MAIL_DBID = "SELECT id FROM mails WHERE uid = :uid AND folder = :folder";
    const std::string INSERT_MAILPART = "INSERT INTO mailparts(mail_id, type, name, encoding, content) "
                                        "VALUES(:mail_id, :type, :name, :encoding, :content)";

    const std::string GET_EMAIL = "SELECT uid, folder, subject, sender_name, sender_email, date, read FROM "
                                  "mails WHERE folder = :folder AND uid = :uid";
    const std::string GET_EMAIL_PARTS = "SELECT mail_id, type, name, encoding, content FROM "
                                        "mailparts WHERE mail_id = :mail_id";

    const std::string GET_ALL_UIDS_FROM_FOLDER = "SELECT uid FROM "
                                                 "mails WHERE folder = :folder ORDER BY uid DESC";

    const std::string MAIL_CACHED = "SELECT COUNT(*) FROM mails WHERE folder = :folder and uid = :uid";
    const std::string LAST_UID_FROM_FOLDER = "SELECT COALESCE(MAX(uid), -1) FROM mails WHERE folder = :folder";

    const std::string FOLDER_COUNT = "SELECT COUNT(*) FROM folders";
    const std::string GET_FOLDER_CANONICAL_NAME = "SELECT canonical_name FROM folders LIMIT 1 OFFSET :offset";
    const std::string GET_FOLDER_READABLE_NAME = "SELECT readable_name FROM folders LIMIT 1 OFFSET :offset";


    const std::string BEGIN_TRANSACTION = "BEGIN TRANSACTION;";
    const std::string END_TRANSACTION = "END TRANSACTION;";
    const std::string ROLLBACK_TRANSACTION = "ROLLBACK TRANSACTION;";

    // One full version bump per entry.
    // Index = version - 1, e.g. index 1 migrates from version 1 to version 2.
    std::vector<std::vector<std::string>> dbMigrationStatements {
        {}, // version 0 -> 1: dummy

        {"ALTER TABLE mails ADD COLUMN sender_name TEXT",
        "ALTER TABLE mails RENAME COLUMN sender TO sender_email",
        "UPDATE mails SET sender_name = SUBSTR(sender_email, 0, INSTR(sender_email, '<'))", // extract name from sender header
        "UPDATE mails SET sender_email = TRIM(TRIM(SUBSTR(sender_email, INSTR(sender_email, '<')), '<'), '>')", // extract email from sender header
        "INSERT INTO settings(key, value) VALUES ('DB_VERSION', '2')"}, // version 1->2

        {"ALTER TABLE folders RENAME COLUMN original_name TO canonical_name",
         "DELETE FROM folders", // have to pull all folder names again
         "UPDATE settings SET value = '3' WHERE key = 'DB_VERSION'"} // version 2->3
    };


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
    sqlite3_stmt* get_all_uids_from_folder_statement;
    sqlite3_stmt* folder_count_statement;
    sqlite3_stmt* canonical_folder_name_statement;
    sqlite3_stmt* readable_folder_name_statement;


    void initializeConnection();
    void initializeTable(const std::string& statement);
    void initializeTables();
    void performUpdateAndMigration();
    void prepareStatements();
    void destroyStatements();
    int getDBVersion();

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
    std::string getFolderName(FolderNameType folderNameType, size_t index);

    DbManager();
public:
    static DbManager* getInstance();
    ~DbManager();
    void storeEmail(const struct Mail& mail);
    bool isMailCached(int uid, std::string folder);

    void storeFolder(const std::string& original_name, const std::string& readable_name);
    bool areFoldersCached();

    int getLastCachedUid(std::string folder);
    Mail fetchMail(std::string folder, int uid, bool includeContent = false);
    std::vector<Mail> getAllMailsFromFolder(std::string folder);

    void registerMailCallback(const std::function<void(void)> cb);
    void registerFolderCallback(const std::function<void(void)> cb);

    std::string getReadableFolderName(size_t index);
    std::string getCanonicalFolderName(size_t index);

    int getFolderCount();
};

#endif // DBMANAGER_H
