import QtQuick
import QtQuick.Controls
import QtWebEngine

Item {
    property alias contentPath: mailWebEngineView.url

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
        settings.javascriptEnabled: false
        settings.localContentCanAccessRemoteUrls: true

        onNavigationRequested: function(request){
            if (request.navigationType == WebEngineView.LinkClickedNavigation){
                request.action = WebEngineView.IgnoreRequest
                Qt.openUrlExternally(request.url)
            }
        }
    }


}
