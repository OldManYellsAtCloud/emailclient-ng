#include "mailmodel.h"
#include "utils.h"

MailModel::MailModel(QObject *parent)
    : QAbstractListModel{parent}
{
    dbManager = DbManager::getInstance();
    roleNames_m[MailModel::subjectRole] = "subject";
    roleNames_m[MailModel::fromRole] = "from";
    roleNames_m[MailModel::bodyRole] = "body";
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
    emit beginRemoveRows(QModelIndex(), 0, mails.size() -1);
    emit endRemoveRows();

    std::vector<Mail> newMails = dbManager->getAllMailsFromFolder(folder.toStdString());
    emit beginInsertRows(QModelIndex(), 0, newMails.size() - 1);
    mails = newMails;
    emit endInsertRows();
}

