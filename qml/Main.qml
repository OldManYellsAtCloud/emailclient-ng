import QtQuick
import QtQuick.Controls
import sgy.gspine.mail

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Mailclient")

    Shortcut {
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()
    }

    ModelFactory {
        id: modelFactory
    }

    PeriodicDataFetcher {
        id: periodicDataFetcher
    }

    StackView {
        id: stackView
        anchors.top: appHeader.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        initialItem: MailList {}
    }

    Drawer {
        id: drawer
        width: 0.66 * parent.width
        height: parent.height - appHeader.height
        y: appHeader.height

        focus: false

        Label {
            id: title
            text: "Menu"
            anchors.top: parent.top

        }

        ListView {
            id: drawerListview
            anchors.fill: parent
            model: modelFactory.getFolderModel();

            delegate: FolderListDelegate {
                text: model.display;
            }
        }
    }

    AppHeader {
        id: appHeader
        folder: modelFactory.getMailModel().currentFolder === "" ? qsTr("No Folder Selected") :
                                                                   modelFactory.getMailModel().currentFolder
        fetchInProgress: periodicDataFetcher.fetchInProgress
        anchors.top: parent.top
    }
}

