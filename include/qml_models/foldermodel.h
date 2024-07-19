#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include <QAbstractListModel>
#include "dbmanager.h"


struct FolderName {
    std::string canonicalName;
    std::basic_string<unsigned char> readableName;
};

class FolderModel : public QAbstractListModel
{
    Q_OBJECT
private:
    DbManager* dbManager;
    QHash<int, QByteArray> roleNames_m;
    std::vector<FolderName> folders_m;
public:
    FolderModel();
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    enum RoleNames {
        DisplayRole = Qt::DisplayRole,
        CanonicalNameRole = Qt::UserRole
    };

public slots:
    void foldersFetched();



};

#endif // FOLDERMODEL_H
