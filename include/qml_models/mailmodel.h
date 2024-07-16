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
    std::string tempFolderPath;

    void mailArrived();
    void clearList();
    Q_PROPERTY(QString currentFolder READ getCurrentFolder NOTIFY currentFolderChanged FINAL)

public:
    explicit MailModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString getCurrentFolder();

    Q_INVOKABLE void switchFolder(QString folder);
    Q_INVOKABLE void prepareMailForOpening(const int &index);

    enum RoleNames {
        subjectRole = Qt::UserRole,
        fromRole = Qt::UserRole + 1,
        dateRole = Qt::UserRole + 2,
        contentPathRole = Qt::UserRole + 3
    };

signals:
    void currentFolderChanged();
};

#endif // MAILMODEL_H
