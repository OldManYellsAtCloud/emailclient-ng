#include "dbmanager.h"
#include <loglibrary.h>
#include "dbexception.h"

DbManager::DbManager() {
    mailSettings = std::make_unique<MailSettings>();
    initializeConnection();
    initializeTables();
    performUpdateAndMigration();
    prepareStatements();
}

DbManager* DbManager::getInstance()
{
    static DbManager* dbManager = new DbManager();
    return dbManager;
}

DbManager::~DbManager()
{
    destroyStatements();

    int ret = sqlite3_close_v2(dbConnection);
    try {
        checkSuccess(ret, SQLITE_OK, "Could not close database connection");
    } catch (DbException e){
        // Don't die here, it's already in the destructor, the
        // application is going down already. checkSuccess logs the error.
    }
    delete getInstance();
}

void DbManager::checkSuccess(int result, int expected_result, std::string info)
{
    if (result != expected_result){
        ERROR("SQLITE ERROR - {} - {}: {}", info, result, sqlite3_errstr(result));
        std::string err = std::format("SQLITE ERROR - {} - {}: {}", info, result, sqlite3_errstr(result));
        throw DbException(err);
    }
}

int DbManager::getParameterIndex(sqlite3_stmt *stmt, std::string parameter_name)
{
    int ret = sqlite3_bind_parameter_index(stmt, parameter_name.c_str());
    if (ret == 0){
        ERROR("Could not find the following parameter in statement: {}", parameter_name);
        throw DbException("Missing parameter in prepared statement!");
    }
    return ret;
}

void DbManager::registerMailCallback(const std::function<void ()> cb)
{
    mailCallbacks.push_back(cb);
}

void DbManager::registerFolderCallback(const std::function<void ()> cb)
{
    folderCallbacks.push_back(cb);
}

int DbManager::getDBVersion()
{
    sqlite3_stmt* dbVersionStatement;
    int ret = sqlite3_prepare_v2(dbConnection, GET_DB_VERSION.c_str(), -1, &dbVersionStatement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare db-version query.");

    ret = sqlite3_step(dbVersionStatement);
    checkSuccess(ret, SQLITE_ROW, "Fatal: could not query current db version");

    std::string versionString = reinterpret_cast<const char*>(sqlite3_column_text(dbVersionStatement, 0));

    int version;
    try {
        version = stoi(versionString);
    } catch (std::exception e){
        ERROR("Could not convert {} to db version. Error: {}", versionString, e.what());
        throw e;
    }

    return version;
}

void DbManager::storeEmail(const Mail &mail)
{

    int ret;
    try {
        ret = sqlite3_exec(dbConnection, BEGIN_TRANSACTION.c_str(), NULL, NULL, NULL);
        checkSuccess(ret, SQLITE_OK, "Could not start transaction");

        storeMailInfo(mail);
        storeMailParts(mail);

        ret = sqlite3_exec(dbConnection, END_TRANSACTION.c_str(), NULL, NULL, NULL);
        checkSuccess(ret, SQLITE_OK, "Could not finish transaction");
        for (const auto& cb: mailCallbacks)
            cb();

    } catch (DbException e){
        ERROR("Unsuccessful transaction: {}", e.what());

        ret = sqlite3_exec(dbConnection, ROLLBACK_TRANSACTION.c_str(), NULL, NULL, NULL);
        checkSuccess(ret, SQLITE_OK, "Fatal: Could not rollback failed transaction");
    }
}

void DbManager::storeMailInfo(const Mail &mail)
{
    const std::lock_guard<std::mutex> lock(dbLock);
    resetStatementAndClearBindings(insert_mail_statement);

    auto getIndex = [&](const std::string& param_name)->int {
        return getParameterIndex(insert_mail_statement, param_name);
    };

    int ret = sqlite3_bind_int(insert_mail_statement, getIndex(":uid"), mail.uid);
    checkSuccess(ret, SQLITE_OK, "Could not bind uid to insert mail statement");

    ret = sqlite3_bind_text(insert_mail_statement, getIndex(":folder"), mail.folder.c_str(),
                            -1, SQLITE_TRANSIENT);
    checkSuccess(ret, SQLITE_OK, "Could not bind folder to insert mail statement");

    ret = sqlite3_bind_text(insert_mail_statement, getIndex(":subject"), mail.subject.c_str(),
                            -1, SQLITE_TRANSIENT);
    checkSuccess(ret, SQLITE_OK, "Could not bind subject to insert mail statement");

    ret = sqlite3_bind_text(insert_mail_statement, getIndex(":sender"), mail.from.c_str(),
                            -1, SQLITE_TRANSIENT);
    checkSuccess(ret, SQLITE_OK, "Could not bind sender to insert mail statement");

    ret = sqlite3_bind_text(insert_mail_statement, getIndex(":date"), mail.date_string.c_str(),
                            -1, SQLITE_TRANSIENT);
    checkSuccess(ret, SQLITE_OK, "Could not bind date to insert mail statement");

    ret = sqlite3_bind_int(insert_mail_statement, getIndex(":read"), true);
    checkSuccess(ret, SQLITE_OK, "Could not bind read to insert mail statement");

    ret = sqlite3_step(insert_mail_statement);
    checkSuccess(ret, SQLITE_DONE, "Could not insert mail into db");
}

void DbManager::storeMailParts(const Mail &mail)
{
    int dbid = getDbId(mail);
    LOG("Mail ID: {}", dbid);

    auto getIndex = [&](const std::string& param_name)->int {
        return getParameterIndex(insert_mailpart_statement, param_name.c_str());
    };

    int ret;
    for (const struct MailPart& mp: mail.parts){
        resetStatementAndClearBindings(insert_mailpart_statement);

        ret = sqlite3_bind_int(insert_mailpart_statement, getIndex(":mail_id"), dbid);
        checkSuccess(ret, SQLITE_OK, "Could not bind id to mailpart insertion statement");

        ret = sqlite3_bind_int(insert_mailpart_statement, getIndex(":type"), mp.ct);
        checkSuccess(ret, SQLITE_OK, "Could not bind content type to mailpart insertion statement");

        ret = sqlite3_bind_text(insert_mailpart_statement, getIndex(":name"), mp.name.c_str(),
                                -1, SQLITE_TRANSIENT);
        checkSuccess(ret, SQLITE_OK, "Could not bind name to mailpart insertion statement");

        ret = sqlite3_bind_int(insert_mailpart_statement, getIndex(":encoding"), mp.enc);
        checkSuccess(ret, SQLITE_OK, "Could not bind encoding to mailpart insetion statement");

        ret = sqlite3_bind_text(insert_mailpart_statement, getIndex(":content"), mp.content.c_str(),
                                -1, SQLITE_TRANSIENT);
        checkSuccess(ret, SQLITE_OK, "Could not bind content to mailpart insertion statement");

        ret = sqlite3_step(insert_mailpart_statement);
        checkSuccess(ret, SQLITE_DONE, "Could not insert mailpart into db");
    }
}

std::vector<int> DbManager::getAllUidsFromFolder(std::string folder)
{
    std::vector<int> uids;
    resetStatementAndClearBindings(get_all_uids_from_folder_statement);
    int ret = sqlite3_bind_text(get_all_uids_from_folder_statement, 1, folder.c_str(),
                                -1, SQLITE_TRANSIENT);
    checkSuccess(ret, SQLITE_OK, "Could not bind folder to get-all-uids statement");

    ret = sqlite3_step(get_all_uids_from_folder_statement);
    if (ret != SQLITE_ROW && ret != SQLITE_DONE){
        throw DbException("Could not query UIDs for folder " + folder);
    }

    while (ret != SQLITE_DONE){
        uids.push_back(sqlite3_column_int(get_all_uids_from_folder_statement, 0));
        ret = sqlite3_step(get_all_uids_from_folder_statement);
    }
    return uids;
}

int DbManager::getDbId(const Mail &mail)
{
    resetStatementAndClearBindings(get_mail_dbid_statement);

    auto getIndex = [&](const std::string& parameter_name)->int {
        return getParameterIndex(get_mail_dbid_statement, parameter_name);
    };

    int ret = sqlite3_bind_int(get_mail_dbid_statement, getIndex(":uid"), mail.uid);
    checkSuccess(ret, SQLITE_OK, "Could not bind uid to getDbid statement");

    ret = sqlite3_bind_text(get_mail_dbid_statement, getIndex(":folder"), mail.folder.c_str(),
                            -1, SQLITE_TRANSIENT);
    checkSuccess(ret, SQLITE_OK, "Could not bind folder to getDbid statement");

    ret = sqlite3_step(get_mail_dbid_statement);
    checkSuccess(ret, SQLITE_ROW, "Could not execute getDbid statement");

    int dbid = sqlite3_column_int(get_mail_dbid_statement, 0);
    return dbid;
}

bool DbManager::isMailCached(int uid, std::string folder)
{
    auto getIndex = [&](const std::string& parameter_name)->int {
        return getParameterIndex(is_mail_cached_statement, parameter_name.c_str());
    };

    int ret, row_num = 0;

    try {
        resetStatementAndClearBindings(is_mail_cached_statement);
        ret = sqlite3_bind_int(is_mail_cached_statement, getIndex(":uid"), uid);
        checkSuccess(ret, SQLITE_OK, "Could not bind uid to mail-cached statement");

        ret = sqlite3_bind_text(is_mail_cached_statement, getIndex(":folder"), folder.c_str(),
                                -1, SQLITE_TRANSIENT);
        checkSuccess(ret, SQLITE_OK, "Could not bind folder to mail-cached statement");

        ret = sqlite3_step(is_mail_cached_statement);
        checkSuccess(ret, SQLITE_ROW, "Could not execute mail-cached statement");
        row_num = sqlite3_column_int(is_mail_cached_statement, 1);
    } catch (DbException e){
        ERROR("Could not count mails. Uid: {}, folder: {}, Error: {}",
              uid, folder, e.what());
    }

    return row_num > 0;
}

void DbManager::storeFolder(const std::string &original_name, const std::string &readable_name)
{
    auto getIndex = [&](const std::string& parameter_name)->int {
        return getParameterIndex(insert_folder_statement, parameter_name.c_str());
    };

    int ret;
    try {
        resetStatementAndClearBindings(insert_folder_statement);
        ret = sqlite3_bind_text(insert_folder_statement, getIndex(":original_name"), original_name.c_str(),
                                -1, SQLITE_TRANSIENT);
        checkSuccess(ret, SQLITE_OK, "Could not bind original name to insert folder statement");

        ret = sqlite3_bind_text(insert_folder_statement, getIndex(":readable_name"), readable_name.c_str(),
                                -1, SQLITE_TRANSIENT);
        checkSuccess(ret, SQLITE_OK, "Could not bind readable name to insert folder statement");

        ret = sqlite3_step(insert_folder_statement);
        checkSuccess(ret, SQLITE_DONE, "Could not insert folder in db");

        for (const auto& cb: folderCallbacks)
            cb();

    } catch (std::exception e){
        ERROR("Could not insert folder name in db: {}", e.what());
    }
}

std::vector<std::pair<std::string, std::string>> DbManager::getFolderNames()
{
    std::vector<std::pair<std::string, std::string>> folders;
    int ret;
    try {
        resetStatementAndClearBindings(select_folders_statement);
        ret = sqlite3_step(select_folders_statement);
        if (ret != SQLITE_OK && ret != SQLITE_DONE && ret != SQLITE_ROW){
            ERROR("Unexpected error while querying folders: {} - {}", ret, sqlite3_errstr(ret));
            throw DbException("Could not query folders");
        }

        while (ret != SQLITE_DONE){
            std::string original_name = reinterpret_cast<const char*>(sqlite3_column_text(select_folders_statement, 0));
            std::string readable_name = reinterpret_cast<const char*>(sqlite3_column_text(select_folders_statement, 1));
            folders.push_back({original_name, readable_name});
            ret = sqlite3_step(select_folders_statement);
        }
    } catch (std::exception e){
        ERROR("Could not query folders: {}", e.what());
    }
    return folders;
}

bool DbManager::areFoldersCached()
{
    // if no folders are present in the table... then they are not present.
    return !getFolderNames().empty();
}

int DbManager::getLastCachedUid(std::string folder)
{
    auto getIndex = [&](const std::string& parameter_name)->int {
        return getParameterIndex(get_last_cached_uid_statement, parameter_name.c_str());
    };

    int lastUid = -1;
    try {
        resetStatementAndClearBindings(get_last_cached_uid_statement);
        int ret = sqlite3_bind_text(get_last_cached_uid_statement, getIndex(":folder"), folder.c_str(),
                                    -1, SQLITE_TRANSIENT);
        checkSuccess(ret, SQLITE_OK, "Could not bind value to get last uid statement");

        ret = sqlite3_step(get_last_cached_uid_statement);
        checkSuccess(ret, SQLITE_ROW, "Could not execute get last uid statement");

        lastUid = sqlite3_column_int(get_last_cached_uid_statement, 0);
    } catch (std::exception e){
        ERROR("Could not get last cached uid: {}", e.what());
    }
    return lastUid;
}

Mail DbManager::fetchMail(std::string folder, int uid, bool includeContent)
{
    const std::lock_guard<std::mutex> lock(dbLock);

    Mail mail;
    auto getEmailIndex = [&](const std::string& parameter_name)->int {
        return getParameterIndex(get_mail_statement, parameter_name.c_str());
    };

    auto getEmailPartIndex = [&](const std::string& parameter_name)->int {
        return getParameterIndex(get_mailpart_statement, parameter_name.c_str());
    };

    try {
        resetStatementAndClearBindings(get_mail_statement);
        resetStatementAndClearBindings(get_mailpart_statement);

        int ret = sqlite3_bind_text(get_mail_statement, getEmailIndex(":folder"), folder.c_str(), -1, SQLITE_TRANSIENT);
        checkSuccess(ret, SQLITE_OK, "Could not bind folder to get email statement");

        ret = sqlite3_bind_int(get_mail_statement, getEmailIndex(":uid"), uid);
        checkSuccess(ret, SQLITE_OK, "Could not bind uid to get email statement");

        ret = sqlite3_step(get_mail_statement);
        checkSuccess(ret, SQLITE_ROW, "Could not execute get email statement");

        mail.uid = sqlite3_column_int(get_mail_statement, 0);
        mail.folder = folder;
        mail.subject = reinterpret_cast<const char*>(sqlite3_column_text(get_mail_statement, 2));
        mail.from = reinterpret_cast<const char*>(sqlite3_column_text(get_mail_statement, 3));
        mail.date_string = reinterpret_cast<const char*>(sqlite3_column_text(get_mail_statement, 4));

        int dbid = getDbId(mail);
        ret = sqlite3_bind_int(get_mailpart_statement, getEmailPartIndex(":mail_id"), dbid);
        checkSuccess(ret, SQLITE_OK, "Could not bind mail_id to get email part statement.");

        if (includeContent){
            ret = sqlite3_step(get_mailpart_statement);
            checkSuccess(ret, SQLITE_ROW, "Could not execute get mailpart statement");


            std::vector<MailPart> mailParts;

            while (ret == SQLITE_ROW){
                struct MailPart mp;
                mp.ct = static_cast<CONTENT_TYPE>(sqlite3_column_int(get_mailpart_statement, 1));
                mp.name = reinterpret_cast<const char*>(sqlite3_column_text(get_mailpart_statement, 2));
                mp.enc = static_cast<ENCODING>(sqlite3_column_int(get_mailpart_statement, 3));
                mp.content = reinterpret_cast<const char*>(sqlite3_column_text(get_mailpart_statement, 4));
                mailParts.push_back(mp);
                ret = sqlite3_step(get_mailpart_statement);
            }
            mail.parts = mailParts;
        }

    } catch (std::exception e){
        mail = {0};
        ERROR("Could not gather mail from database: {}", e.what());
    }
    return mail;
}

std::vector<Mail> DbManager::getAllMailsFromFolder(std::string folder)
{
    std::vector<Mail> mails;
    std::vector<int> uids = getAllUidsFromFolder(folder);
    for (const int& uid: uids)
        mails.push_back(fetchMail(folder, uid));

    return mails;

}


void DbManager::resetStatementAndClearBindings(sqlite3_stmt *statement)
{
    int ret = sqlite3_reset(statement);
    checkSuccess(ret, SQLITE_OK, "Could not reset statement");

    ret = sqlite3_clear_bindings(statement);
    checkSuccess(ret, SQLITE_OK, "Could not clear statement bindings");
}

void DbManager::initializeConnection()
{
    std::string dbPath = mailSettings->getDbPath();

    int ret = sqlite3_open_v2(dbPath.c_str(), &dbConnection, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: Could not open database");
}

void DbManager::initializeTables()
{
    initializeTable(CREATE_MAIL_TABLE);
    initializeTable(CREATE_MAILPARTS_TABLE);
    initializeTable(CREATE_INDEX);
    initializeTable(CREATE_SETTINGS_TABLE);
    initializeTable(CREATE_FOLDERS_TABLE);
}

void DbManager::performUpdateAndMigration()
{
    int currentDbVersion = getDBVersion();
    if (currentDbVersion == LATEST_DB_VERSION){
        LOG("Db is already latest version, no migration needed");
        return;
    }

    LOG("Performing db migration from version {} to {}", currentDbVersion, LATEST_DB_VERSION);

    int ret;

    for (int i = currentDbVersion; i < dbMigrationStatements.size(); ++i){
        for (const std::string& statement: dbMigrationStatements[i]){
            sqlite3_stmt* dbUpdateStatement;
            ret = sqlite3_prepare_v2(dbConnection, statement.c_str(), -1, &dbUpdateStatement, NULL);
            checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare migration query: " + statement);

            ret = sqlite3_step(dbUpdateStatement);
            checkSuccess(ret, SQLITE_DONE, "Fatal: could not execute migration query: " + statement);
            sqlite3_finalize(dbUpdateStatement);
        }
    }

    LOG("Db migration successful");
}

void DbManager::prepareStatements()
{
    int ret = sqlite3_prepare_v2(dbConnection, INSERT_MAIL.c_str(), -1, &insert_mail_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare insert mail statement");

    ret = sqlite3_prepare_v2(dbConnection, INSERT_MAILPART.c_str(), -1, &insert_mailpart_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare insert mail part statement");

    ret = sqlite3_prepare_v2(dbConnection, GET_MAIL_DBID.c_str(), -1, &get_mail_dbid_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare get dbid statement");

    ret = sqlite3_prepare_v2(dbConnection, MAIL_CACHED.c_str(), -1, &is_mail_cached_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare mail-cached statement");

    ret = sqlite3_prepare_v2(dbConnection, LAST_UID_FROM_FOLDER.c_str(), -1, &get_last_cached_uid_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare last cached uid statement");

    ret = sqlite3_prepare_v2(dbConnection, GET_EMAIL.c_str(), -1, &get_mail_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare get email statement");

    ret = sqlite3_prepare_v2(dbConnection, GET_EMAIL_PARTS.c_str(), -1, &get_mailpart_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare get email part statement");

    ret = sqlite3_prepare_v2(dbConnection, INSERT_FOLDER.c_str(), -1, &insert_folder_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare insert folder statement");

    ret = sqlite3_prepare_v2(dbConnection, SELECT_FOLDERS.c_str(), -1, &select_folders_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare folder query statement");

    ret = sqlite3_prepare_v2(dbConnection, GET_ALL_UIDS_FROM_FOLDER.c_str(), -1, &get_all_uids_from_folder_statement, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: could not prepare get-uids-from-folder statement");
}

void DbManager::destroyStatements()
{
    sqlite3_finalize(insert_mail_statement);
    sqlite3_finalize(insert_mailpart_statement);
    sqlite3_finalize(get_mail_dbid_statement);
    sqlite3_finalize(is_mail_cached_statement);
    sqlite3_finalize(get_last_cached_uid_statement);
    sqlite3_finalize(get_mail_statement);
    sqlite3_finalize(get_mailpart_statement);
    sqlite3_finalize(insert_folder_statement);
    sqlite3_finalize(select_folders_statement);
    sqlite3_finalize(get_all_uids_from_folder_statement);
}

void DbManager::initializeTable(const std::string& statement)
{
    int ret = sqlite3_exec(dbConnection, statement.c_str(), NULL, NULL, NULL);
    checkSuccess(ret, SQLITE_OK, "Fatal: Could not execute table init statement");
}

