#ifndef MODELFACTORY_H
#define MODELFACTORY_H

#include <QQmlEngine>
#include "curlrequestscheduler.h"
#include "qml_models/foldermodel.h"
#include <QAbstractListModel>

class ModelFactory: public QObject
{
    Q_OBJECT
    QML_ELEMENT
private:
    CurlRequestScheduler curlRequestScheduler;
    ImapRequest imapRequest;
    FolderModel folderModel;

public:
    ModelFactory();
    Q_INVOKABLE QAbstractListModel* getFolderModel();
};

#endif // MODELFACTORY_H
