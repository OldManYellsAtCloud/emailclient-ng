#include <loglibrary.h>
#include "mailmodel.h"
#include "utils.h"
#include "mailsettings.h"

MailModel::MailModel(QObject *parent)
    : QAbstractListModel{parent}
{
    currentFolder = "";
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

    if (role == MailModel::subjectRole){
        ret = QString::fromStdString(decodeSender(mails[index.row()].subject));
    } else if (role == MailModel::fromRole){
        ret = QString::fromStdString(decodeSender(mails[index.row()].sender_name));
    } else if (role == MailModel::dateRole){
        ret = QString::fromStdString(mails[index.row()].date_string);
    } else if (role == MailModel::contentPathRole){
        std::string contentExtension = mailHasHTMLPart(mails[index.row()]) ? ".html" : ".txt";
        ret = QString::fromStdString("file://" + tempFolderPath + contentExtension);
    } else {
        return QVariant();
    }

    DBG("MailModel - Returning data: {}", ret.toStdString());

    return ret;
}

QHash<int, QByteArray> MailModel::roleNames() const
{
    return roleNames_m;
}

QString MailModel::getCurrentFolder()
{
    return QString::fromStdString(currentFolder);
}

void MailModel::switchFolder(QString folder)
{
    currentFolder = folder.toStdString();
    emit currentFolderChanged();
    clearList();

    std::vector<Mail> newMails = dbManager->getAllMailsFromFolder(currentFolder);
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
    if (currentFolder.empty())
        return;

    std::vector<Mail> newMails = dbManager->getAllMailsFromFolder(currentFolder);
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
