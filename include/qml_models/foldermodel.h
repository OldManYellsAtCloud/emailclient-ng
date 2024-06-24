#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include <QAbstractListModel>
#include "dbmanager.h"

class FolderModel : public QAbstractListModel
{
    Q_OBJECT
private:
    DbManager* dbManager;
    std::vector<std::string> folders_m;

    std::vector<std::string> parseFolderResponse(std::string response);
public:
    FolderModel();
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

public slots:
    void foldersFetched();
};

#endif // FOLDERMODEL_H
