import QtQuick
import QtQuick.Controls
import QtWebEngine

Item {
    Button {
        id: back
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        text: "Back"
        onClicked: {
            stackView.pop()
        }
    }

    WebEngineView {
        id: mailWebEngineView
        anchors.top: back.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        url: "file:///tmp/mymails/index.html"
        settings.javascriptEnabled: false
        settings.localContentCanAccessRemoteUrls: true
    }
}
