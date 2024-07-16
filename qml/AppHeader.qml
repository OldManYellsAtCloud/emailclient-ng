import QtQuick
import QtQuick.Window

Rectangle  {
    property alias folder: folderName.text
    property alias fetchInProgress: refreshAnimation.running

    height: Screen.height * 0.03
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    color: "light grey"

    Text {
        id: folderName
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        font.pixelSize: Screen.height * 0.02
        font.bold: true
        color: "dark green"
    }

    Image {
        id: refreshIcon
        height: parent.height
        width: height
        source: "qrc:/qml/res/small_refresh_icon"

        RotationAnimation{
            id: refreshAnimation
            duration: 1000
            from: 360
            to: 0
            target: refreshIcon
            loops: RotationAnimation.Infinite
        }
    }
}
