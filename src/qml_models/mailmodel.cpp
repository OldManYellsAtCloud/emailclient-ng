#include "mailmodel.h"
#include "utils.h"


MailModel::MailModel(QObject *parent)
    : QAbstractListModel{parent}
{
    currentFolder = "";
    dbManager = DbManager::getInstance();
    roleNames_m[MailModel::subjectRole] = "subject";
    roleNames_m[MailModel::fromRole] = "from";
    roleNames_m[MailModel::dateRole] = "date";

    auto newMailCallback = [&](){this->mailArrived();};
    dbManager->registerMailCallback(newMailCallback);
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

    switch (role){
    case MailModel::subjectRole:
        ret = QString::fromStdString(decodeSender(mails[index.row()].subject));
        return ret;
    case MailModel::fromRole:
        ret = QString::fromStdString(decodeSender(mails[index.row()].from));
        return ret;
    case MailModel::dateRole:
        ret = QString::fromStdString(mails[index.row()].date_string);
        return ret;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> MailModel::roleNames() const
{
    return roleNames_m;
}

void MailModel::switchFolder(QString folder)
{
    currentFolder = folder.toStdString();
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

    writeMailToDisk(mails[index]);
}

void MailModel::mailArrived()
{
    if (currentFolder.empty())
        return;

    std::vector<Mail> newMails = dbManager->getAllMailsFromFolder(currentFolder);
    if (newMails.size() == mails.size())
        return;

    clearList();
    emit beginInsertRows(QModelIndex(), 0, newMails.size() - 1);
    mails = newMails;
    emit endInsertRows();
}

void MailModel::clearList()
{
    emit beginRemoveRows(QModelIndex(), 0, mails.size() -1);
    emit endRemoveRows();
}
