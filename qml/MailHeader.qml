import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

Rectangle {
    id: mailListRoot
    anchors.left: parent ? parent.left : undefined
    anchors.right: parent ? parent.right : undefined

    height: 65
    radius: 2

    property alias recipient_or_sender: recipient_or_sender.text
    property alias subject: subject.text
    property alias date: date.text

    border.color: "grey"
    border.width: 1

    Text {
        id: recipient_or_sender
        font.pixelSize: 15
        font.bold: true
        color: "black"
        anchors.top: parent.top
        anchors.right: parent.right
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideLeft
    }

    Text {
        id: subject
        font.bold: false
        font.pixelSize: 15
        color: "dark blue"
        anchors.top: recipient_or_sender.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        elide: Text.ElideRight
    }

    Text {
        id: date
        font.bold: false
        font.pixelSize: 15
        color: "green"
        anchors.top: subject.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        horizontalAlignment: Text.AlignRight
    }


    MouseArea {
        anchors.fill: parent
        onClicked: {
            modelFactory.getMailModel().prepareMailForOpening(model.index)
            stackView.push("MailContent.qml")

        }
    }
}
