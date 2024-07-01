import QtQuick

Item {
    id: root
    ListView {
        id: mailListView
        anchors.fill: parent
        model: modelFactory.getMailModel()
        delegate: MailHeader {
            recipient_or_sender: model.from
            subject: model.subject
            date: model.date
        }
    }
}
