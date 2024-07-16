#include "qml_models/modelfactory.h"

ModelFactory::ModelFactory(): curlRequestScheduler(&curlRequest) {

}

QAbstractListModel* ModelFactory::getFolderModel()
{
    return &folderModel;
}

QAbstractListModel *ModelFactory::getMailModel()
{
    return &mailModel;
}


