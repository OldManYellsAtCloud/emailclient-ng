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

    Drawer {
        id: drawer
        width: 0.66 * parent.width
        height: parent.height
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
}

