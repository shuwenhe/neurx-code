import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import NeurXCode

// ── ExplorerContextMenu ────────────────────────────────────────────────────
// Context menu for file explorer right-click operations
// Based on VSCode's file explorer context menu

Menu {
    id: contextMenu

    required property var panel
    property string targetPath: ""
    property string targetName: ""
    property bool targetIsDirectory: false

    signal createFileRequested(string dirPath)
    signal createFolderRequested(string dirPath)
    signal renameRequested(string path)
    signal deleteRequested(string path)
    signal copyRequested(string path)
    signal cutRequested(string path)
    signal pasteRequested(string dirPath)

    MenuItem {
        text: "New File"
        onTriggered: contextMenu.createFileRequested(
            contextMenu.targetIsDirectory ? contextMenu.targetPath : ""
        )
    }

    MenuItem {
        text: "New Folder"
        onTriggered: contextMenu.createFolderRequested(
            contextMenu.targetIsDirectory ? contextMenu.targetPath : ""
        )
    }

    MenuSeparator {}

    MenuItem {
        text: "Rename"
        onTriggered: contextMenu.renameRequested(contextMenu.targetPath)
    }

    MenuItem {
        text: "Delete"
        onTriggered: contextMenu.deleteRequested(contextMenu.targetPath)
    }

    MenuSeparator {}

    MenuItem {
        text: "Copy"
        onTriggered: contextMenu.copyRequested(contextMenu.targetPath)
    }

    MenuItem {
        text: "Cut"
        onTriggered: contextMenu.cutRequested(contextMenu.targetPath)
    }

    MenuItem {
        text: "Paste"
        enabled: false  // TODO: Check if clipboard has content
        onTriggered: contextMenu.pasteRequested(
            contextMenu.targetIsDirectory ? contextMenu.targetPath : ""
        )
    }

    MenuSeparator {}

    MenuItem {
        text: "Copy Path"
        onTriggered: {
            if (contextMenu.panel && contextMenu.panel.agent) {
                contextMenu.panel.agent.copyToClipboard(contextMenu.targetPath)
            }
        }
    }

    MenuItem {
        text: "Reveal in Finder"
        visible: Qt.platform.os === "osx"
        onTriggered: {
            if (contextMenu.panel && contextMenu.panel.agent) {
                contextMenu.panel.agent.revealInFinder(contextMenu.targetPath)
            }
        }
    }

    MenuItem {
        text: "Open in Terminal"
        onTriggered: {
            if (contextMenu.panel && contextMenu.panel.agent) {
                const dir = contextMenu.targetIsDirectory ? 
                    contextMenu.targetPath : 
                    contextMenu.targetPath.substring(0, contextMenu.targetPath.lastIndexOf("/"))
                contextMenu.panel.agent.openInTerminal(dir)
            }
        }
    }

    function open(path, name, isDir, position) {
        contextMenu.targetPath = path
        contextMenu.targetName = name
        contextMenu.targetIsDirectory = isDir
        popup(position.x, position.y)
    }
}
