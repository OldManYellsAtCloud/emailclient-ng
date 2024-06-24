#include "qml_models/modelfactory.h"

ModelFactory::ModelFactory(): curlRequestScheduler(&imapRequest) {

}

QAbstractListModel* ModelFactory::getFolderModel()
{
    return &folderModel;
}


