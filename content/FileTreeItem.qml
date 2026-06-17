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

    // Directory contents model
    property var directoryItems: []

    function updateDirectoryContents() {
        if (!root.panel || !root.panel.agent || !root.dirPath) {
            root.directoryItems = []
            return
        }
        root.directoryItems = root.panel.agent.listDirectoryContents(root.dirPath) || []
    }

    onDirPathChanged: updateDirectoryContents()
    
    Component.onCompleted: updateDirectoryContents()

    ColumnLayout {
        id: column
        width: root.width
        spacing: 6

        Label {
            Layout.fillWidth: true
            text: root.depth === 0 ? "Workspace Files" : "Folder"
            font.bold: true
            color: Theme.textPrimary
            visible: root.depth === 0 && root.directoryItems.length > 0
            font.pixelSize: Theme.fontSm
        }

        // Show directory contents
        Column {
            Layout.fillWidth: true
            spacing: 0
            visible: root.depth === 0 && root.directoryItems.length > 0

            Repeater {
                model: root.directoryItems

                delegate: ItemDelegate {
                    width: parent.width
                    height: 24
                    padding: 0
                    leftPadding: 8
                    rightPadding: 8
                    
                    text: modelData.name
                    
                    background: Rectangle {
                        color: parent.hovered ? Theme.surfaceAlt : "transparent"
                        radius: 2
                    }

                    contentItem: Text {
                        text: parent.text
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontXs
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        if (modelData.isDirectory) {
                            console.log("Folder clicked:", modelData.name)
                        } else {
                            root.fileClicked(modelData.path)
                        }
                    }
                }
            }
        }

        // Show "No files" or recent files if directory is empty
        Label {
            Layout.fillWidth: true
            text: root.depth === 0 && root.directoryItems.length === 0 && root.panel && root.panel.agent && root.panel.agent.workspaceRecentFiles.length > 0
                ? "Recent Files"
                : root.depth === 0 && root.directoryItems.length === 0
                ? "No files in workspace"
                : ""
            color: Theme.textMuted
            font.pixelSize: Theme.fontXs
            visible: root.depth === 0
        }

        // Recent files list (shown only if no directory contents)
        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: false
            implicitHeight: contentHeight
            clip: true
            model: root.depth === 0 && root.directoryItems.length === 0
                ? (root.panel && root.panel.agent ? root.panel.agent.workspaceRecentFiles : [])
                : []

            delegate: ItemDelegate {
                width: ListView.view.width
                height: 24
                text: modelData
                
                contentItem: Text {
                    text: parent.text
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontXs
                    elide: Text.ElideRight
                }
                
                background: Rectangle {
                    color: parent.hovered ? Theme.surfaceAlt : "transparent"
                    radius: 2
                }
                
                onClicked: root.fileClicked(modelData)
            }
        }
    }
}
