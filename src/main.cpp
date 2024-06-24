#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtWebEngineQuick/QtWebEngineQuick>

#include <pwd.h>

#include "qml_models/modelfactory.h"
#include "periodicdatafetcher.h"
#include <loglibrary.h>



void checkProcessOwner(int argc, char *argv[]){
    MailSettings settings{};
    std::string applicationUser = settings.getApplicationUser();
    auto currentUid = getuid();
    auto currentUser = getpwuid(currentUid);

    std::string currentUserName {currentUser->pw_name};

    if (currentUserName.compare(applicationUser) != 0){
        LOG("Running with incorrect user: {} Attempting to switch to application user.", currentUserName);
        auto appUserUid = getpwnam(applicationUser.c_str());

        if (!appUserUid){
            ERROR("Could not find application user! Error: {}", strerror(errno));
            exit(1);
        }

        if (setgid(appUserUid->pw_gid) < 0){
            ERROR("Could not set GID to application user's group! Error: {}", strerror(errno));
            exit(1);
        }

        if(setuid(appUserUid->pw_uid) < 0){
            ERROR("Could not set UID to application user! Error: {}", strerror(errno));
            exit(1);
        }

        execv(argv[0], argv);
    } else {
        LOG("Already running as application user.");
    }
}

int main(int argc, char *argv[])
{
    checkProcessOwner(argc, argv);
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
