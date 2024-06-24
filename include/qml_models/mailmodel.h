#ifndef MAILMODEL_H
#define MAILMODEL_H

#include <QAbstractListModel>
#include <QQmlEngine>
#include "dbmanager.h"

class MailModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
private:
    DbManager* dbManager;
    std::vector<Mail> mails;
    QHash<int, QByteArray> roleNames_m;
    std::string currentFolder;
    void mailArrived();
public:
    explicit MailModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void switchFolder(QString folder);
    Q_INVOKABLE void prepareMailForOpening(const int &index);

    enum RoleNames {
        subjectRole = Qt::UserRole,
        fromRole = Qt::UserRole + 1,
        bodyRole = Qt::UserRole + 2
    };
};

#endif // MAILMODEL_H
