#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebEngineQuick/QtWebEngineQuick>


#include "qml_models/modelfactory.h"
#include "periodicdatafetcher.h"

int main(int argc, char *argv[])
{
    qmlRegisterType<ModelFactory>("sgy.gspine.mail", 1, 0, "ModelFactory");
    qmlRegisterType<PeriodicDataFetcher>("sgy.gspine.mail", 1, 0, "PeriodicDataFetcher");
    QtWebEngineQuick::initialize();
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/emailclient/qml/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
