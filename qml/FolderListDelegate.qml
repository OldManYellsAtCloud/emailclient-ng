import QtQuick

Rectangle {
    property alias text: folderLabel.text
    width: parent ? parent.width : undefined
    height: 30
    Text {
        id: folderLabel
        font.pixelSize: 20
        clip: true
        elide: Text.ElideRight
        color: "steelblue"
        padding: 3
        width: parent.width
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            var canonicalName = model.canonicalName;
            periodicDataFetcher.fetchFolder(canonicalName);
            modelFactory.getMailModel().switchFolder(canonicalName)
            drawer.close()
        }
    }
}
