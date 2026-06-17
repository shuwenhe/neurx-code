import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import NeurXCode

// ── FileOperationDialog ────────────────────────────────────────────────────
// Unified dialog for file operations: create, rename, delete
// Based on VSCode's file operation dialogs

Dialog {
    id: fileOpDialog

    required property var agent
    
    property string operationMode: ""  // "new-file", "new-folder", "rename", "delete"
    property string targetPath: ""
    property string targetName: ""
    property string targetDirPath: ""

    signal operationCompleted(string mode, string result)
    signal operationCancelled()

    modal: true
    implicitWidth: 420
    closePolicy: Popup.CloseOnEscape

    title: {
        if (operationMode === "new-file") return "New File"
        if (operationMode === "new-folder") return "New Folder"
        if (operationMode === "rename") return "Rename"
        if (operationMode === "delete") return "Delete"
        return "File Operation"
    }

    onOpened: {
        if (operationMode === "delete") {
            deleteConfirmButton.forceActiveFocus()
        } else {
            nameField.text = targetName || ""
            nameField.forceActiveFocus()
            nameField.selectAll()
        }
    }

    contentItem: ColumnLayout {
        width: 380
        spacing: 12

        // New File/Folder instructions
        Label {
            Layout.fillWidth: true
            text: {
                if (fileOpDialog.operationMode === "new-file") 
                    return "Enter a name for the new file."
                if (fileOpDialog.operationMode === "new-folder")
                    return "Enter a name for the new folder."
                if (fileOpDialog.operationMode === "rename")
                    return "Enter a new name for the item."
                if (fileOpDialog.operationMode === "delete")
                    return "Delete \"" + fileOpDialog.targetName + "\"? This action cannot be undone."
                return ""
            }
            color: operationMode === "delete" ? Theme.error : Theme.textMuted
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.fontXs
        }

        // Input field (hidden for delete)
        TextField {
            id: nameField
            Layout.fillWidth: true
            visible: fileOpDialog.operationMode !== "delete"
            placeholderText: {
                if (fileOpDialog.operationMode === "new-file")
                    return "filename.ext"
                if (fileOpDialog.operationMode === "new-folder")
                    return "folder_name"
                if (fileOpDialog.operationMode === "rename")
                    return "new name"
                return "name"
            }
            onAccepted: {
                fileOpDialog.accept()
            }
        }

        Item { Layout.fillHeight: true }
    }

    footer: Rectangle {
        color: "transparent"
        implicitHeight: 56

        RowLayout {
            anchors { fill: parent; margins: 12 }
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                onClicked: {
                    fileOpDialog.reject()
                    operationCancelled()
                }
            }

            Button {
                id: confirmButton
                text: {
                    if (fileOpDialog.operationMode === "delete")
                        return "Delete"
                    if (fileOpDialog.operationMode === "rename")
                        return "Rename"
                    return "Create"
                }
                highlighted: true
                onClicked: fileOpDialog.accept()
            }

            Button {
                id: deleteConfirmButton
                visible: false  // Reference button for focus
            }
        }
    }

    onAccepted: {
        const name = nameField.text.trim()
        
        if (operationMode === "new-file" || operationMode === "new-folder") {
            if (!name) {
                console.log("[FileOperationDialog] Name is empty")
                return
            }
            if (agent && agent.createWorkspaceEntry) {
                const ok = agent.createWorkspaceEntry(targetDirPath, name, 
                    operationMode === "new-folder")
                if (ok) {
                    operationCompleted(operationMode, name)
                }
            }
        } else if (operationMode === "rename") {
            if (!name || name === targetName) {
                console.log("[FileOperationDialog] No name change")
                return
            }
            if (agent && agent.renameWorkspacePath) {
                const ok = agent.renameWorkspacePath(targetPath, name)
                if (ok) {
                    operationCompleted(operationMode, name)
                }
            }
        } else if (operationMode === "delete") {
            if (agent && agent.deleteWorkspacePath) {
                const ok = agent.deleteWorkspacePath(targetPath)
                if (ok) {
                    operationCompleted(operationMode, targetPath)
                }
            }
        }
    }

    onRejected: {
        operationCancelled()
    }

    function openCreateFile(dirPath) {
        operationMode = "new-file"
        targetDirPath = dirPath
        targetName = "untitled.txt"
        open()
    }

    function openCreateFolder(dirPath) {
        operationMode = "new-folder"
        targetDirPath = dirPath
        targetName = "untitled_folder"
        open()
    }

    function openRename(path) {
        operationMode = "rename"
        targetPath = path
        targetName = path.split("/").pop()
        open()
    }

    function openDelete(path) {
        operationMode = "delete"
        targetPath = path
        targetName = path.split("/").pop()
        open()
    }
}
