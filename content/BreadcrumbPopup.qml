import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import NeurXCode

Popup {
    id: root
    width: 280
    height: 260
    padding: 0
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string path: ""
    signal itemClicked(string filePath, bool isDir)

    background: Rectangle {
        color: Theme.surface
        radius: Theme.radius
        border.color: Theme.border
        border.width: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Label {
            Layout.fillWidth: true
            text: root.path || "Breadcrumbs"
            padding: 10
            color: Theme.textPrimary
            elide: Text.ElideMiddle
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: []
            delegate: ItemDelegate {
                width: ListView.view.width
                text: modelData
            }
        }
    }
}
