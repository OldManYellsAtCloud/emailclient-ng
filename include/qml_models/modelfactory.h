#ifndef MODELFACTORY_H
#define MODELFACTORY_H

#include <QQmlEngine>
#include "curlrequestscheduler.h"
#include "qml_models/foldermodel.h"
#include "qml_models/mailmodel.h"
#include <QAbstractListModel>

class ModelFactory: public QObject
{
    Q_OBJECT
    QML_ELEMENT
private:
    CurlRequestScheduler curlRequestScheduler;
    ImapRequest imapRequest;
    FolderModel folderModel;
    MailModel mailModel;

public:
    ModelFactory();
    Q_INVOKABLE QAbstractListModel* getFolderModel();
    Q_INVOKABLE QAbstractListModel* getMailModel();
};

#endif // MODELFACTORY_H
