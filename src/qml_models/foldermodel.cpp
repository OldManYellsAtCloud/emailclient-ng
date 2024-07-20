#include "qml_models/foldermodel.h"
#include <loglibrary.h>

FolderModel::FolderModel() {
    dbManager = DbManager::getInstance();
    auto dbCallback = [&](){this->foldersFetched();};
    dbManager->registerFolderCallback(dbCallback);

    foldersFetched();
}

int FolderModel::rowCount(const QModelIndex &parent) const
{
    return dbManager->getFolderCount();
}

QVariant FolderModel::data(const QModelIndex &index, int role) const
{
    DBG("Data requested for index {}", index.row());
    if (index.row() < 0 || index.row() >= dbManager->getFolderCount() || role != Qt::DisplayRole)
        return QVariant();

    auto folderName = dbManager->getReadableFolderName(index.row());
    return QString::fromLatin1(folderName.data());
}


void FolderModel::foldersFetched()
{
    int folderCount = dbManager->getFolderCount();
    emit beginInsertRows(QModelIndex(), 0, folderCount - 1);
    emit endInsertRows();
}
