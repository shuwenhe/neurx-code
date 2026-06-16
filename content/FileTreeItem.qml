import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import NeurXCode

Item {
    id: root

    required property var panel
    property string dirPath: ""
    property int depth: 0
    property string filterText: ""

    signal fileClicked(string path)

    implicitHeight: column.implicitHeight

    ColumnLayout {
        id: column
        width: root.width
        spacing: 6

        Label {
            Layout.fillWidth: true
            text: root.depth === 0
                ? "Workspace browser is using the simplified view."
                : "Nested folders are not available in this environment."
            color: Theme.textMuted
            wrapMode: Text.WordWrap
            visible: root.depth === 0
        }

        Label {
            Layout.fillWidth: true
            text: root.panel && root.panel.agent && root.panel.agent.workspaceRecentFiles.length
                ? "Recent files"
                : "No recent files"
            color: Theme.textMuted
            visible: root.depth === 0
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: contentHeight
            clip: true
            model: root.panel && root.panel.agent ? root.panel.agent.workspaceRecentFiles : []

            delegate: ItemDelegate {
                width: ListView.view.width
                text: modelData
                onClicked: root.fileClicked(modelData)
            }
        }
    }
}
