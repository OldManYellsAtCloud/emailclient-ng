#include <loglib/loglib.h>
#include "mailmodel.h"
#include "utils.h"
#include "mailsettings.h"

MailModel::MailModel(QObject *parent)
    : QAbstractListModel{parent}
{
    currentFolderIndex = -1;
    dbManager = DbManager::getInstance();
    roleNames_m[MailModel::subjectRole] = "subject";
    roleNames_m[MailModel::fromRole] = "from";
    roleNames_m[MailModel::dateRole] = "date";
    roleNames_m[MailModel::contentPathRole] = "contentPath";

    auto newMailCallback = [&](){this->mailArrived();};
    dbManager->registerMailCallback(newMailCallback);

    tempFolderPath = MailSettings().getTempFolder();
}

int MailModel::rowCount(const QModelIndex &parent) const
{
    return mails.size();
}

QVariant MailModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= mails.size())
        return QVariant();

    QString ret;
    std::string tmp;

    if (role == MailModel::subjectRole){
        tmp = unquoteString(mails[index.row()].subject);
        tmp = decodeSender(tmp);
        ret = QString::fromStdString(tmp);
    } else if (role == MailModel::fromRole){
        tmp = unquoteString(mails[index.row()].sender_name);
        if (tmp.empty()) tmp = mails[index.row()].sender_email;
        tmp = decodeSender(tmp);
        ret = QString::fromStdString(tmp);
    } else if (role == MailModel::dateRole){
        ret = QString::fromStdString(mails[index.row()].date_string);
    } else if (role == MailModel::contentPathRole){
        std::string contentExtension = mailHasHTMLPart(mails[index.row()]) ? "html" : "txt";
        ret = QString::fromStdString("file://" + tempFolderPath + "/index." + contentExtension);
    } else {
        return QVariant();
    }

    LOG_DEBUG_F("MailModel - Returning data: {}", ret.toStdString());

    return ret;
}

QHash<int, QByteArray> MailModel::roleNames() const
{
    return roleNames_m;
}

QString MailModel::getCurrentFolder()
{
    if (currentFolderIndex < 0)
        return QString();
    return QString::fromLatin1(dbManager->getReadableFolderName(currentFolderIndex).data());
}

void MailModel::switchFolder(int folderIndex)
{
    currentFolderIndex = folderIndex;
    emit currentFolderChanged();
    clearList();

    std::string canonicalFolderName = dbManager->getCanonicalFolderName(currentFolderIndex);

    std::vector<Mail> newMails = dbManager->getAllMailsFromFolder(canonicalFolderName);
    emit beginInsertRows(QModelIndex(), 0, newMails.size() - 1);
    mails = newMails;
    emit endInsertRows();
}

void MailModel::prepareMailForOpening(const int &index)
{
    if (index < 0 || index > mails.size())
        return;

    if (!mails[index].arePartsAvailable()){
        std::string folder = mails[index].folder;
        int uid = mails[index].uid;
        bool fetchMailParts = true;
        mails[index] = dbManager->fetchMail(folder, uid, fetchMailParts);
    }

    writeMailToDisk(mails[index], tempFolderPath);
}

void MailModel::mailArrived()
{
    if (currentFolderIndex < 0)
        return;

    std::string canonicalFolderName = dbManager->getCanonicalFolderName(currentFolderIndex);
    std::vector<Mail> newMails = dbManager->getAllMailsFromFolder(canonicalFolderName);
    if (newMails.size() == mails.size())
        return;

    emit beginInsertRows(QModelIndex(), 0, newMails.size() - mails.size() - 1);
    mails = newMails;
    emit endInsertRows();
}

void MailModel::clearList()
{
    emit beginRemoveRows(QModelIndex(), 0, mails.size() - 1);
    emit endRemoveRows();
}
