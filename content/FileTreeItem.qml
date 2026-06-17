import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import NeurXCode

Item {
    id: root

    required property var panel
    required property string path
    required property string name
    property bool isDirectory: false
    property bool isExpanded: false
    property bool hasChildren: false
    property bool isSearchMatch: false
    property bool isCurrentFile: false
    property bool isSelected: false
    property bool isDropTarget: false
    property bool dragActive: false
    property int depth: 0

    signal fileClicked(string path)
    signal folderNavigationRequested(string folderPath)

    implicitHeight: row.implicitHeight

    readonly property bool dragEnabled: !!root.path && !!root.panel && !!root.panel.agent

    Drag.active: dragHandler.active && root.dragEnabled
    Drag.source: root
    Drag.keys: ["application/x-neurx-filetree"]
    Drag.supportedActions: Qt.MoveAction
    Drag.dragType: Drag.Automatic
    Drag.hotSpot.x: width / 2
    Drag.hotSpot.y: height / 2

    function accentColor(alpha) {
        return Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, alpha)
    }

    function mutedColor(alpha) {
        return Qt.rgba(Theme.textMuted.r, Theme.textMuted.g, Theme.textMuted.b, alpha)
    }

    function openFolderMenuAction(actionName) {
        const targetDir = root.isDirectory ? root.path : root.panel.currentDirForPath(root.path)
        if (actionName === "new-file") {
            root.panel.openCreateDialog(targetDir, false)
        } else if (actionName === "new-folder") {
            root.panel.openCreateDialog(targetDir, true)
        } else if (actionName === "rename") {
            root.panel.openRenameDialog(root.path)
        } else if (actionName === "delete") {
            root.panel.openDeleteDialog(root.path)
        } else if (actionName === "find-in-folder") {
            root.panel.findInFolderRequested(targetDir)
        }
    }

    Rectangle {
        id: row
        width: root.width
        implicitHeight: 24
        radius: 3
        color: root.isSelected
               ? root.accentColor(root.isCurrentFile ? 0.24 : 0.18)
               : (root.isCurrentFile
                  ? root.accentColor(0.14)
                  : (root.isDropTarget ? root.accentColor(0.10) : (mouseArea.containsMouse ? Theme.surfaceAlt : "transparent")))
        border.color: root.isSelected
                      ? Theme.accent
                      : (root.isDropTarget ? Theme.accent : (root.isSearchMatch ? Theme.accent : "transparent"))
        border.width: root.isSelected || root.isDropTarget || root.isSearchMatch ? 1 : 0
        opacity: root.dragActive ? 0.75 : 1.0

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.isDropTarget ? 2 : 0
            color: Theme.accent
            visible: height > 0
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: root.isDropTarget && root.isDirectory ? "Release to move" : ""
            color: Theme.accent
            font.pixelSize: Theme.fontXs
            visible: text.length > 0
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8 + (root.depth * 16)
            anchors.rightMargin: 6
            spacing: 6

            Item {
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12

                Rectangle {
                    x: 1
                    y: 4
                    width: 10
                    height: 7
                    radius: 1.5
                    color: root.isDirectory ? (root.isCurrentFile ? Theme.accent : root.mutedColor(0.72)) : "transparent"
                    border.color: root.isDirectory ? root.accentColor(root.isCurrentFile ? 0.9 : 0.45) : "transparent"
                    border.width: root.isDirectory ? 1 : 0
                }

                Rectangle {
                    x: 2
                    y: 2
                    width: 5
                    height: 3
                    radius: 1
                    color: root.isDirectory ? (root.isCurrentFile ? Theme.accent : root.mutedColor(0.86)) : "transparent"
                }

                Rectangle {
                    x: 7
                    y: 0
                    width: 3
                    height: 4
                    radius: 0.5
                    color: root.isDirectory ? root.accentColor(root.isCurrentFile ? 0.72 : 0.35) : "transparent"
                }
            }

            Item {
                Layout.preferredWidth: 12
                Layout.preferredHeight: 14

                Rectangle {
                    x: 1
                    y: 1
                    width: 10
                    height: 12
                    radius: 1
                    color: root.isDirectory ? "transparent" : root.mutedColor(0.14)
                    border.color: root.isDirectory
                                  ? "transparent"
                                  : (root.isCurrentFile ? Theme.accent : root.mutedColor(0.60))
                    border.width: root.isDirectory ? 0 : 1
                }

                Rectangle {
                    x: 7
                    y: 1
                    width: 4
                    height: 4
                    rotation: 45
                    transformOrigin: Item.TopLeft
                    color: root.isDirectory ? "transparent" : (root.isCurrentFile ? Theme.accent : root.mutedColor(0.34))
                }

                Rectangle {
                    x: 7
                    y: 1
                    width: 1
                    height: 12
                    color: root.isDirectory ? "transparent" : (root.isCurrentFile ? Theme.accent : root.mutedColor(0.42))
                }

                Rectangle {
                    x: 1
                    y: 6
                    width: 10
                    height: 1
                    color: root.isDirectory ? "transparent" : (root.isCurrentFile ? Theme.accent : root.mutedColor(0.28))
                }
            }

            Text {
                text: root.isDirectory ? (root.isExpanded ? "▾" : "▸") : " "
                color: root.isDirectory ? Theme.textMuted : "transparent"
                font.pixelSize: Theme.fontXs
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                Layout.fillWidth: true
                text: root.name
                color: root.isCurrentFile ? Theme.textPrimary : (root.isSearchMatch ? Theme.accent : Theme.textPrimary)
                font.pixelSize: Theme.fontXs
                font.bold: root.isDirectory || root.isCurrentFile
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }
        }

        DragHandler {
            id: dragHandler
            target: null
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: {
                root.dragActive = active
                if (!active && root.panel)
                    root.panel.clearDropTarget()
            }
        }

        DropArea {
            anchors.fill: parent
            enabled: root.isDirectory
            keys: ["application/x-neurx-filetree"]

            onEntered: (drag) => {
                if (!root.isDirectory)
                    return
                const source = drag.source
                if (source && source.path && source.path !== root.path && root.panel) {
                    root.panel.setDropTarget(root.path)
                    if (root.isDirectory && !root.isExpanded) {
                        expandTimer.restart()
                    }
                }
            }

            onExited: {
                expandTimer.stop()
                if (root.panel && root.panel.dropTargetPath === root.path)
                    root.panel.clearDropTarget(root.path)
            }

            onDropped: (drop) => {
                expandTimer.stop()
                const source = drop.source
                const sourcePath = source && source.path ? source.path : ""
                if (!sourcePath || sourcePath === root.path || !root.panel)
                    return

                if (root.panel.movePathIntoDirectory(sourcePath, root.path)) {
                    drop.acceptProposedAction()
                    root.panel.clearDropTarget()
                }
            }
        }

        Timer {
            id: expandTimer
            interval: 350
            repeat: false
            onTriggered: {
                if (root.isDirectory && !root.isExpanded && root.panel)
                    root.panel.setExpanded(root.path, true)
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: root.isDirectory ? Qt.PointingHandCursor : Qt.ArrowCursor

            onClicked: (mouse) => {
                if (root.panel)
                    root.panel.forceActiveFocus()
                if (mouse.button === Qt.RightButton) {
                    contextMenu.targetPath = root.path
                    contextMenu.isDirectory = root.isDirectory
                    contextMenu.canPaste = root.panel.hasClipboardItem()
                    contextMenu.popup()
                    return
                }

                if (root.isDirectory) {
                    root.panel.selectPath(root.path)
                    root.folderNavigationRequested(root.path)
                } else {
                    root.panel.selectPath(root.path)
                    root.fileClicked(root.path)
                }
            }

            onDoubleClicked: (mouse) => {
                if (mouse.button !== Qt.LeftButton)
                    return
                if (root.panel)
                    root.panel.forceActiveFocus()
                if (root.isDirectory)
                    root.folderNavigationRequested(root.path)
                else
                    root.fileClicked(root.path)
            }
        }
    }

    FileTreeContextMenu {
        id: contextMenu

        onNewFile: root.openFolderMenuAction("new-file")
        onNewFolder: root.openFolderMenuAction("new-folder")
        onRename: root.openFolderMenuAction("rename")
        onDeleteItem: root.openFolderMenuAction("delete")
        onCopyPath: root.panel.copyPathToClipboard(root.path)
        onCopyRelativePath: root.panel.copyRelativePathToClipboard(root.path)
        onCut: root.panel.cutPathToClipboard(root.path)
        onCopy: root.panel.copyPathEntryToClipboard(root.path)
        onPaste: root.panel.pasteClipboardInto(root.isDirectory ? root.path : root.panel.currentDirForPath(root.path))
        onFindInFolder: root.openFolderMenuAction("find-in-folder")
    }
}
