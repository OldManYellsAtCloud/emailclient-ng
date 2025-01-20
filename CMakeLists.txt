cmake_minimum_required(VERSION 3.16)

project(emailclient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Quick WebEngineQuick Network)
find_package(CURL REQUIRED)
find_package(loglib REQUIRED)
find_package(SettingsLib REQUIRED)
find_package(SQLite3 REQUIRED)

set(SOURCES src/imap/curlrequest.cpp
            src/imap/curlresponse.cpp
            src/mailsettings.cpp
            src/imap/imapmailparser.cpp
            src/utils.cpp
            src/base64.cpp
            src/dbmanager.cpp
            src/dbexception.cpp
            src/curlrequestscheduler.cpp
            src/imap/imapfetcher.cpp
            src/qml_models/foldermodel.cpp
            src/qml_models/mailmodel.cpp
            src/qml_models/modelfactory.cpp
            src/periodicdatafetcher.cpp
)

set(HEADERS include/imap/curlrequest.h
            include/imap/curlresponse.h
            include/imap/imapfetcher.h
            include/mailsettings.h
            include/imap/imapmailparser.h
            include/utils.h
            include/base64.h
            include/mailpart.h
            include/mail.h
            include/dbmanager.h
            include/dbexception.h
            include/curlrequestscheduler.h
            include/qml_models/foldermodel.h
            include/qml_models/modelfactory.h
            include/qml_models/mailmodel.h
            include/periodicdatafetcher.h
            include/imap/imaprequestinterface.h
)

qt_standard_project_setup()

qt_add_executable(appemailclient
    src/main.cpp
    ${HEADERS}
    ${SOURCES}
)

qt_add_qml_module(appemailclient
    URI emailclient
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        QML_FILES qml/FolderListDelegate.qml
        QML_FILES qml/MailHeader.qml
        QML_FILES qml/MailList.qml
        QML_FILES qml/MailContent.qml
        QML_FILES qml/AppHeader.qml
)

qt_add_resources(appemailclient imageResources FILES qml/res/small_refresh_icon.png)



target_link_libraries(appemailclient
    PRIVATE Qt6::Quick curl SettingsLib loglib SQLite::SQLite3 Qt6::WebEngineQuick Qt6::Network
)

target_include_directories(appemailclient PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/include/qml_models)

include(GNUInstallDirs)
install(TARGETS appemailclient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)


option(enable_test "Compile and run tests" OFF)

if (enable_test)
    find_package(GTest REQUIRED)
    enable_testing()

    include(GoogleTest)


    set(TEST_SOURCES tests/dev_tests.cpp)

    add_executable(email_tests ${HEADERS} ${SOURCES} ${TEST_SOURCES})

    target_include_directories(email_tests PRIVATE include)
    target_include_directories(email_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

    target_link_directories(email_tests PRIVATE $ENV{CMAKE_SYSROOT}/usr/lib)
    target_link_libraries(email_tests PRIVATE GTest::gtest_main GTest::gmock_main SettingsLib Qt6::Quick curl SQLite::SQLite3 Qt6::WebEngineQuick)

    install(TARGETS email_tests
            BUNDLE DESTINATION .
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DTEST_ENABLED")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTEST_ENABLED")
endif()
