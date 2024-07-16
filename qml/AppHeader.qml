import QtQuick
import QtQuick.Window

Rectangle  {
    property alias folder: folderName.text

    height: Screen.height * 0.03
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    color: "light grey"

    Text {
        id: folderName
        anchors.right: parent.right
        font.pixelSize: Screen.height * 0.02
        font.bold: true
        color: "dark green"
    }
}
