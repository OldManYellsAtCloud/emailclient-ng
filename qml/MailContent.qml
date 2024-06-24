import QtQuick
import QtQuick.Controls
import QtWebView

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

    WebView {
        id: mailWebView
        anchors.top: back.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        url: "file:///tmp/mymails/index.html"
        settings.javaScriptEnabled: false
    }
}
