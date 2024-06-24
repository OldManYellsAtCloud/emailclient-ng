#include "qml_models/foldermodel.h"


FolderModel::FolderModel() {
    dbManager = DbManager::getInstance();
    auto dbCallback = [&](){this->foldersFetched();};
    dbManager->registerFolderCallback(dbCallback);

    foldersFetched();
}

int FolderModel::rowCount(const QModelIndex &parent) const
{
    return folders_m.size();
}

QVariant FolderModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= folders_m.size() || role != Qt::DisplayRole)
        return QVariant();

    return QString::fromStdString(folders_m[index.row()]);
}

void FolderModel::foldersFetched()
{
    std::vector<std::pair<std::string, std::string>> new_folders = dbManager->getFolderNames();
    std::vector<std::string> folderNames;
    for (std::pair<std::string, std::string>& folder: new_folders)
        folderNames.push_back(folder.second);

    emit beginInsertRows(QModelIndex(), 0, new_folders.size() - 1);
    folders_m = folderNames;
    emit endInsertRows();
}
