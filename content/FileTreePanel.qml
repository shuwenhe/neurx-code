import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import NeurXCode

// ── FileTreePanel ─────────────────────────────────────────────────────────────
//  Explorer sidebar with VS Code-style folder expansion, search filtering,
//  and workspace file actions.

Item {
    id: root
    focus: true

    required property var agent

    signal fileClicked(string path)
    signal findInFolderRequested(string path)

    property var expandedPaths: ({})
    property var searchExpandedPaths: ({})
    property var searchMatches: []
    property string searchFocusPath: ""
    property string filterText: ""
    property string pendingActionMode: ""
    property string pendingTargetPath: ""
    property string pendingTargetDirPath: ""
    property string pendingName: ""
    property string pendingDeletePath: ""
    property string pendingDeleteName: ""
    property string clipboardPath: ""
    property string clipboardMode: ""
    property string dropTargetPath: ""
    property string selectedPath: ""
    property var visibleTreeEntries: []
    property bool entryDialogVisible: entryDialog.visible
    property bool deleteDialogVisible: deleteDialog.visible

    // Root path for the browser.
    property string diskRoot: ""
    property string expandedPathsByWorkspaceJson: "{}"

    function expandedPathsByWorkspace() {
        try {
            return JSON.parse(root.expandedPathsByWorkspaceJson || "{}")
        } catch (e) {
            return {}
        }
    }

    function normalizedPath(path) {
        if (path === undefined || path === null)
            return ""
        let text = path.toString()
        try {
            text = decodeURIComponent(text)
        } catch (e) {
        }
        if (text.startsWith("file://"))
            text = text.slice(7)
        return text.replace(/\\/g, "/")
    }

    function saveExpandedPaths() {
        const workspace = root.agent && root.agent.workspacePath ? root.agent.workspacePath : ""
        const byWorkspace = expandedPathsByWorkspace()
        byWorkspace[workspace] = Object.keys(expandedPaths)
        root.expandedPathsByWorkspaceJson = JSON.stringify(byWorkspace)
    }

    function restoreExpandedPaths() {
        const workspace = root.agent && root.agent.workspacePath ? root.agent.workspacePath : ""
        const byWorkspace = expandedPathsByWorkspace()
        const saved = byWorkspace[workspace] || []
        const next = {}
        for (let i = 0; i < saved.length; ++i)
            next[saved[i]] = true
        expandedPaths = next
    }

    function isExpanded(path) {
        return !!expandedPaths[path]
    }

    function setExpanded(path, expanded) {
        const next = Object.assign({}, expandedPaths)
        if (expanded)
            next[path] = true
        else
            delete next[path]
        expandedPaths = next
        saveExpandedPaths()
        refreshVisibleTree()
    }

    function clearExpandedPaths() {
        expandedPaths = ({})
        saveExpandedPaths()
        refreshVisibleTree()
    }

    function isSearchExpanded(path) {
        return !!searchExpandedPaths[path]
    }

    function currentDirForPath(path) {
        path = normalizedPath(path)
        if (!path)
            return root.agent && root.agent.workspacePath ? root.agent.workspacePath : ""
        const idx = path.lastIndexOf("/")
        if (idx <= 0)
            return root.agent && root.agent.workspacePath ? root.agent.workspacePath : ""
        return path.slice(0, idx)
    }

    function baseName(path) {
        path = normalizedPath(path)
        if (!path)
            return ""
        const parts = path.split("/")
        return parts[parts.length - 1]
    }

    function hasClipboardItem() {
        return !!clipboardPath
    }

    function selectPath(path) {
        selectedPath = normalizedPath(path)
        if (selectedPath)
            Qt.callLater(() => scrollPathIntoView(selectedPath))
    }

    function isVisiblePath(path) {
        const normalized = normalizedPath(path)
        for (let i = 0; i < visibleTreeEntries.length; ++i) {
            if (visibleTreeEntries[i].path === normalized)
                return true
        }
        return false
    }

    function visibleIndexForPath(path) {
        const normalized = normalizedPath(path)
        for (let i = 0; i < visibleTreeEntries.length; ++i) {
            if (visibleTreeEntries[i].path === normalized)
                return i
        }
        return -1
    }

    function visibleEntryAt(index) {
        if (index < 0 || index >= visibleTreeEntries.length)
            return null
        return visibleTreeEntries[index]
    }

    function currentVisibleEntry() {
        let entry = visibleEntryAt(visibleIndexForPath(selectedPath))
        if (!entry)
            entry = visibleEntryAt(visibleIndexForPath(root.agent ? root.agent.currentFilePath : ""))
        if (!entry)
            entry = visibleEntryAt(0)
        return entry
    }

    function ensureSelection() {
        if (selectedPath && isVisiblePath(selectedPath))
            return

        const currentEntry = currentVisibleEntry()
        if (currentEntry) {
            selectedPath = currentEntry.path
        } else if (!selectedPath) {
            selectedPath = normalizedPath(root.diskRoot)
        }
    }

    function setSelectionByIndex(index) {
        const entry = visibleEntryAt(index)
        if (!entry)
            return
        selectPath(entry.path)
    }

    function setSelectionByPath(path) {
        const normalized = normalizedPath(path)
        if (!normalized)
            return
        selectedPath = normalized
        Qt.callLater(() => scrollPathIntoView(normalized))
    }

    function moveSelection(delta) {
        if (!visibleTreeEntries.length)
            return

        let index = visibleIndexForPath(selectedPath)
        if (index < 0)
            index = delta > 0 ? -1 : visibleTreeEntries.length

        index = Math.max(0, Math.min(visibleTreeEntries.length - 1, index + delta))
        setSelectionByIndex(index)
    }

    function parentPath(path) {
        return currentDirForPath(path)
    }

    function activateSelectedEntry() {
        const entry = currentVisibleEntry()
        if (!entry)
            return

        if (entry.isDirectory) {
            if (isExpanded(entry.path))
                setExpanded(entry.path, false)
            else
                setExpanded(entry.path, true)
            setSelectionByPath(entry.path)
            return
        }

        setSelectionByPath(entry.path)
        root.fileClicked(entry.path)
    }

    function collapseSelectedEntry() {
        const entry = currentVisibleEntry()
        if (!entry)
            return

        if (entry.isDirectory && isExpanded(entry.path)) {
            setExpanded(entry.path, false)
            setSelectionByPath(entry.path)
            return
        }

        const parent = parentPath(entry.path)
        if (parent && parent !== entry.path)
            setSelectionByPath(parent)
    }

    function expandSelectedEntry() {
        const entry = currentVisibleEntry()
        if (!entry)
            return

        if (!entry.isDirectory)
            return

        if (!isExpanded(entry.path)) {
            setExpanded(entry.path, true)
            setSelectionByPath(entry.path)
            return
        }

        const firstChild = visibleEntryAt(visibleIndexForPath(entry.path) + 1)
        if (firstChild && firstChild.depth > entry.depth)
            setSelectionByPath(firstChild.path)
    }

    function setDropTarget(path) {
        dropTargetPath = normalizedPath(path)
    }

    function clearDropTarget(path) {
        if (path === undefined || path === null || path === "" || !dropTargetPath || normalizedPath(path) === dropTargetPath)
            dropTargetPath = ""
    }

    function clearClipboard() {
        clipboardPath = ""
        clipboardMode = ""
    }

    function copyPathEntryToClipboard(path) {
        clipboardPath = normalizedPath(path)
        clipboardMode = "copy"
    }

    function cutPathToClipboard(path) {
        clipboardPath = normalizedPath(path)
        clipboardMode = "cut"
    }

    function copyRelativePathToClipboard(path) {
        const absPath = normalizedPath(path)
        const workspace = root.agent && root.agent.workspacePath ? normalizedPath(root.agent.workspacePath) : ""
        let relative = absPath
        const workspacePrefix = workspace ? (workspace.endsWith("/") ? workspace : workspace + "/") : ""
        if (workspace && (absPath === workspace || absPath.startsWith(workspacePrefix))) {
            relative = absPath.slice(workspace.length).replace(/^\/+/, "")
        }
        if (root.agent && root.agent.copyPathToClipboard)
            root.agent.copyPathToClipboard(relative)
    }

    function pasteClipboardInto(dirPath) {
        if (!clipboardPath || !root.agent)
            return false

        const destinationDir = normalizedPath(dirPath || root.agent.workspacePath || "")
        if (!destinationDir)
            return false

        let ok = false
        if (clipboardMode === "cut") {
            ok = root.agent.moveWorkspacePath(clipboardPath, destinationDir)
            if (ok)
                clearClipboard()
        } else {
            ok = root.agent.copyWorkspacePath(clipboardPath, destinationDir)
        }

        if (ok)
            refreshAfterWorkspaceMutation(destinationDir)
        return ok
    }

    function movePathIntoDirectory(sourcePath, destinationDir) {
        if (!root.agent)
            return false

        const source = normalizedPath(sourcePath)
        const dest = normalizedPath(destinationDir)
        if (!source || !dest)
            return false

        const ok = root.agent.moveWorkspacePath(source, dest)
        if (ok)
            refreshAfterWorkspaceMutation(dest)
        return ok
    }

    function openCreateDialog(dirPath, directory) {
        pendingActionMode = directory ? "new-folder" : "new-file"
        pendingTargetDirPath = normalizedPath(dirPath || (root.agent && root.agent.workspacePath) || "")
        pendingTargetPath = ""
        pendingName = directory ? "new_folder" : "new_file.txt"
        entryDialog.open()
    }

    function openRenameDialog(path) {
        pendingActionMode = "rename"
        pendingTargetPath = normalizedPath(path)
        pendingTargetDirPath = currentDirForPath(path)
        pendingName = baseName(path)
        entryDialog.open()
    }

    function openDeleteDialog(path) {
        pendingDeletePath = normalizedPath(path)
        pendingDeleteName = baseName(path)
        deleteDialog.open()
    }

    function submitEntryDialog() {
        const name = nameField.text.trim()
        if (!name || !root.agent)
            return

        let ok = false
        if (pendingActionMode === "rename") {
            ok = root.agent.renameWorkspacePath(pendingTargetPath, name)
        } else {
            ok = root.agent.createWorkspaceEntry(pendingTargetDirPath, name, pendingActionMode === "new-folder")
        }

        if (ok) {
            if (pendingActionMode !== "rename")
                setExpanded(pendingTargetDirPath, true)
            refreshAfterWorkspaceMutation(pendingTargetDirPath)
            entryDialog.close()
        }
    }

    function submitDeleteDialog() {
        if (!root.agent)
            return

        const ok = root.agent.deleteWorkspacePath(pendingDeletePath)
        if (ok) {
            refreshAfterWorkspaceMutation(currentDirForPath(pendingDeletePath))
            deleteDialog.close()
        }
    }

    function refreshAfterWorkspaceMutation(path) {
        clearDropTarget()
        updateSearchExpansion()
        refreshVisibleTree()
        revealCurrentFile()
        if (path)
            setExpanded(path, true)
    }

    function updateSearchExpansion() {
        const query = root.filterText.trim()
        if (!query || !root.agent || !root.agent.workspacePath) {
            searchExpandedPaths = ({})
            searchMatches = []
            searchFocusPath = ""
            refreshVisibleTree()
            return
        }

        const matches = root.agent.searchWorkspacePaths(query)
        searchMatches = matches
        searchFocusPath = matches.length ? matches[0] : ""

        const next = {}
        const workspace = normalizedPath(root.agent.workspacePath)
        for (let i = 0; i < matches.length; ++i) {
            const filePath = normalizedPath(matches[i])
            if (!filePath.startsWith(workspace))
                continue

            const rel = filePath.slice(workspace.length).replace(/^\/+/, "")
            if (!rel)
                continue

            const parts = rel.split("/")
            let current = workspace
            for (let j = 0; j < parts.length - 1; ++j) {
                current += "/" + parts[j]
                next[current] = true
            }
        }

        searchExpandedPaths = next
        refreshVisibleTree()
        if (searchFocusPath)
            searchFocusTimer.restart()
    }

    function expandAncestors(filePath) {
        if (!filePath || !root.agent || !root.agent.workspacePath)
            return

        const workspace = normalizedPath(root.agent.workspacePath)
        const absPath = normalizedPath(filePath)
        if (!absPath.startsWith(workspace))
            return

        const rel = absPath.slice(workspace.length).replace(/^\/+/, "")
        if (!rel)
            return

        const parts = rel.split("/")
        let current = workspace
        for (let i = 0; i < parts.length - 1; ++i) {
            current += "/" + parts[i]
            setExpanded(current, true)
        }
    }

    function ensureCurrentFileRoot(filePath) {
        if (!filePath)
            return false

        const currentRoot = normalizedPath(root.diskRoot || "/")
        const normalizedFilePath = normalizedPath(filePath)
        if (normalizedFilePath === currentRoot || normalizedFilePath.startsWith(currentRoot.endsWith("/") ? currentRoot : currentRoot + "/"))
            return false

        const workspace = root.agent && root.agent.workspacePath ? normalizedPath(root.agent.workspacePath) : ""
        if (workspace && (normalizedFilePath === workspace || normalizedFilePath.startsWith(workspace + "/"))) {
            root.diskRoot = workspace
            return true
        }

        root.diskRoot = "/"
        return true
    }

    function findObjectByName(item, name) {
        if (!item)
            return null
        if (item.objectName === name)
            return item
        const children = item.children || []
        for (let i = 0; i < children.length; ++i) {
            const found = findObjectByName(children[i], name)
            if (found)
                return found
        }
        return null
    }

    function scrollPathIntoView(path) {
        const marker = findObjectByName(treeFlick.contentItem, normalizedPath(path))
        treeFlick.currentFileMarker = marker
        if (!marker)
            return

        const pos = marker.mapToItem(treeFlick.contentItem, 0, 0)
        const target = Math.max(0, pos.y - treeFlick.height / 2 + marker.height / 2)
        treeFlick.contentY = Math.max(0, Math.min(target, treeFlick.contentHeight - treeFlick.height))
    }

    function scrollCurrentFileIntoView() {
        scrollPathIntoView(root.agent ? root.agent.currentFilePath : "")
    }

    function revealCurrentFile() {
        if (!root.agent)
            return
        const changedRoot = ensureCurrentFileRoot(root.agent.currentFilePath)
        expandAncestors(root.agent.currentFilePath)
        if (changedRoot)
            Qt.callLater(() => Qt.callLater(() => scrollCurrentFileIntoView()))
        else
            Qt.callLater(() => scrollCurrentFileIntoView())
    }

    function sortEntries(entries) {
        return entries.slice().sort((a, b) => {
            if (a.isDirectory !== b.isDirectory)
                return a.isDirectory ? -1 : 1
            return a.name.localeCompare(b.name, undefined, { numeric: true, sensitivity: "base" })
        })
    }

    function listChildren(path) {
        if (!root.agent || !path)
            return []
        const children = root.agent.listDirectoryContents(path) || []
        const normalized = []
        for (let i = 0; i < children.length; ++i) {
            const child = children[i]
            normalized.push({
                name: child.name || baseName(child.path),
                path: normalizedPath(child.path),
                isDirectory: !!child.isDirectory,
                size: child.size || 0,
                isSymLink: !!child.isSymLink
            })
        }
        return sortEntries(normalized)
    }

    function pathMatchesQuery(entryPath, entryName, query) {
        if (!query)
            return true
        const needle = query.toLowerCase()
        return normalizedPath(entryPath).toLowerCase().includes(needle)
            || (entryName || "").toLowerCase().includes(needle)
    }

    function hasMatchingDescendant(path, query, cache) {
        const cached = cache[path]
        if (cached !== undefined)
            return cached

        const children = listChildren(path)
        for (let i = 0; i < children.length; ++i) {
            const child = children[i]
            if (pathMatchesQuery(child.path, child.name, query)) {
                cache[path] = true
                return true
            }
            if (child.isDirectory && hasMatchingDescendant(child.path, query, cache)) {
                cache[path] = true
                return true
            }
        }

        cache[path] = false
        return false
    }

    function buildVisibleTreeEntries(path, depth, cache) {
        const result = []
        if (!root.agent || !path)
            return result

        const query = root.filterText.trim()
        const children = listChildren(path)
        for (let i = 0; i < children.length; ++i) {
            const child = children[i]
            const matches = pathMatchesQuery(child.path, child.name, query)
            const descendantMatch = child.isDirectory && query ? hasMatchingDescendant(child.path, query, cache) : false
            if (query && !matches && !descendantMatch)
                continue

            const childExpanded = child.isDirectory && (isExpanded(child.path) || isSearchExpanded(child.path))
            const isCurrentFile = root.agent && normalizedPath(root.agent.currentFilePath) === child.path
            result.push({
                name: child.name,
                path: child.path,
                isDirectory: child.isDirectory,
                isExpanded: childExpanded,
                hasChildren: child.isDirectory && listChildren(child.path).length > 0,
                isSearchMatch: query ? matches : false,
                isCurrentFile: isCurrentFile,
                depth: depth
            })

            if (child.isDirectory && childExpanded) {
                const nested = buildVisibleTreeEntries(child.path, depth + 1, cache)
                for (let j = 0; j < nested.length; ++j)
                    result.push(nested[j])
            }
        }
        return result
    }

    function refreshVisibleTree() {
        if (!root.agent || !root.diskRoot) {
            visibleTreeEntries = []
            return
        }

        const cache = {}
        const normalized = normalizedPath(root.diskRoot)
        visibleTreeEntries = buildVisibleTreeEntries(normalized, 0, cache)
        ensureSelection()
    }

    function toggleFolderExpanded(path) {
        const normalized = normalizedPath(path)
        setExpanded(normalized, !isExpanded(normalized))
    }

    function goUpOneLevel() {
        const current = normalizedPath(root.diskRoot)
        if (!current || current === "/")
            return

        const parts = current.replace(/\/$/, "").split("/")
        parts.pop()
        const next = parts.length <= 1 ? "/" : parts.join("/")
        root.diskRoot = next || "/"
    }

    Keys.onPressed: (event) => {
        if (!visibleTreeEntries.length)
            return

        switch (event.key) {
        case Qt.Key_Down:
            moveSelection(1)
            event.accepted = true
            break
        case Qt.Key_Up:
            moveSelection(-1)
            event.accepted = true
            break
        case Qt.Key_Left:
            collapseSelectedEntry()
            event.accepted = true
            break
        case Qt.Key_Right:
            expandSelectedEntry()
            event.accepted = true
            break
        case Qt.Key_Return:
        case Qt.Key_Enter:
            activateSelectedEntry()
            event.accepted = true
            break
        case Qt.Key_Home:
            setSelectionByIndex(0)
            event.accepted = true
            break
        case Qt.Key_End:
            setSelectionByIndex(visibleTreeEntries.length - 1)
            event.accepted = true
            break
        default:
            break
        }
    }

    Timer {
        id: searchFocusTimer
        interval: 0
        repeat: false
        onTriggered: scrollPathIntoView(searchFocusPath)
    }

    Dialog {
        id: entryDialog
        modal: true
        implicitWidth: 360
        title: pendingActionMode === "rename"
            ? "Rename"
            : (pendingActionMode === "new-folder" ? "New Folder" : "New File")
        closePolicy: Popup.CloseOnEscape
        onOpened: {
            nameField.text = pendingName
            nameField.forceActiveFocus()
            nameField.selectAll()
        }

        contentItem: ColumnLayout {
            width: 320
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: pendingActionMode === "rename"
                      ? "Enter a new name."
                      : "Enter a name for the new item."
                color: Theme.textMuted
                wrapMode: Text.WordWrap
            }

            TextField {
                id: nameField
                objectName: "treeNameField"
                Layout.fillWidth: true
                placeholderText: pendingActionMode === "rename" ? "Rename to..." : "Name"
                onAccepted: root.submitEntryDialog()
            }
        }

        footer: Rectangle {
            color: "transparent"
            implicitHeight: 56

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    onClicked: entryDialog.close()
                }

                Button {
                    text: pendingActionMode === "rename" ? "Rename" : "Create"
                    highlighted: true
                    onClicked: root.submitEntryDialog()
                }
            }
        }
    }

    Dialog {
        id: deleteDialog
        modal: true
        implicitWidth: 380
        title: "Delete"
        closePolicy: Popup.CloseOnEscape
        onOpened: deleteConfirmButton.forceActiveFocus()

        contentItem: ColumnLayout {
            width: 340
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: "Delete \"" + pendingDeleteName + "\"? This cannot be undone."
                color: Theme.textMuted
                wrapMode: Text.WordWrap
            }
        }

        footer: Rectangle {
            color: "transparent"
            implicitHeight: 56

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                Item { Layout.fillWidth: true }

                Button {
                    text: "Cancel"
                    onClicked: deleteDialog.close()
                }

                Button {
                    id: deleteConfirmButton
                    text: "Delete"
                    highlighted: true
                    onClicked: root.submitDeleteDialog()
                }
            }
        }
    }

    Connections {
        target: root.agent
        enabled: !!root.agent
        function onCurrentFilePathChanged() {
            if (!root.agent) return
            root.selectedPath = normalizedPath(root.agent.currentFilePath)
            refreshVisibleTree()
            revealCurrentFile()
        }
        function onWorkspacePathChanged() {
            if (!root.agent) return
            root.diskRoot = root.agent.workspacePath ? normalizedPath(root.agent.workspacePath) : "/"
            restoreExpandedPaths()
            updateSearchExpansion()
            refreshVisibleTree()
            revealCurrentFile()
        }
    }

    onFilterTextChanged: updateSearchExpansion()
    onDiskRootChanged: refreshVisibleTree()

    Component.onCompleted: {
        // Prioritize agent's workspace path
        if (root.agent && root.agent.workspacePath) {
            root.diskRoot = normalizedPath(root.agent.workspacePath)
        } else {
            // Only fallback to saved value if it's reasonable
            const saved = normalizedPath(root.diskRoot || "/")
            if (saved && saved !== "/" && saved.startsWith("/")) {
                root.diskRoot = saved
            } else {
                // Don't set to "/" to avoid security issues with listDirectoryContents
                // Wait for workspacePathChanged signal
                return
            }
        }
        
        restoreExpandedPaths()
        selectedPath = normalizedPath(root.agent ? root.agent.currentFilePath : "")
        updateSearchExpansion()
        refreshVisibleTree()
        revealCurrentFile()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                height: 36
                color: Theme.surfaceAlt

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 6
                    spacing: 4

                    ToolButton {
                        text: "↑"
                        font.pixelSize: Theme.fontSm
                        padding: 2
                        enabled: normalizedPath(root.diskRoot) !== "/"
                        opacity: enabled ? 1.0 : 0.35
                        onClicked: goUpOneLevel()
                        ToolTip.text: "Go up one level"
                        ToolTip.visible: hovered
                    }

                    Label {
                        Layout.fillWidth: true
                        text: normalizedPath(root.diskRoot) === "/"
                              ? "LOCAL DISK  /"
                              : baseName(root.diskRoot).toUpperCase() + "  " + normalizedPath(root.diskRoot)
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSm
                        font.bold: true
                        elide: Text.ElideRight
                    }

                    ToolButton {
                        text: "+"
                        font.pixelSize: Theme.fontSm
                        padding: 2
                        enabled: !!root.agent && !!root.agent.workspacePath
                        onClicked: openCreateDialog(normalizedPath(root.diskRoot), false)
                        ToolTip.text: "New File"
                        ToolTip.visible: hovered
                    }

                    ToolButton {
                        text: "⌁"
                        font.pixelSize: Theme.fontSm
                        padding: 2
                        enabled: !!root.agent && !!root.agent.workspacePath
                        onClicked: openCreateDialog(normalizedPath(root.diskRoot), true)
                        ToolTip.text: "New Folder"
                        ToolTip.visible: hovered
                    }

                    ToolButton {
                        text: "↻"
                        font.pixelSize: Theme.fontSm
                        padding: 2
                        enabled: !!root.agent
                        onClicked: refreshVisibleTree()
                        ToolTip.text: "Refresh"
                        ToolTip.visible: hovered
                    }

                    ToolButton {
                        text: "▾"
                        font.pixelSize: Theme.fontSm
                        padding: 2
                        enabled: Object.keys(expandedPaths).length > 0
                        onClicked: clearExpandedPaths()
                        ToolTip.text: "Collapse All"
                        ToolTip.visible: hovered
                    }

                    ToolButton {
                        text: "⌂"
                        font.pixelSize: Theme.fontSm
                        padding: 2
                        visible: !!root.agent && !!root.agent.workspacePath
                        onClicked: root.diskRoot = normalizedPath(root.agent.workspacePath)
                        ToolTip.text: "Jump to workspace"
                        ToolTip.visible: hovered
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 26
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                Layout.topMargin: 4
                Layout.bottomMargin: 2
                color: Theme.bg
                border.color: searchField.activeFocus ? Theme.accent : Theme.border
                border.width: 1
                radius: 2

                TextField {
                    id: searchField
                    objectName: "treeFilterField"
                    anchors.fill: parent
                    anchors.margins: 1
                    placeholderText: "Filter (e.g. .cpp)"
                    text: root.filterText
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSm
                    font.family: Theme.uiFont.family
                    leftPadding: 8
                    background: Item {}
                    onTextEdited: {
                        root.filterText = text
                        root.updateSearchExpansion()
                    }
                }
            }

            Flickable {
                id: treeFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: treeColumn.implicitHeight
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: CustomScrollBar {}
                ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AlwaysOff }

                property Item currentFileMarker: null

                DropArea {
                    anchors.fill: parent
                    keys: ["application/x-neurx-filetree"]

                    onEntered: (drag) => {
                        const source = drag.source
                        if (source && source.path && source.path !== normalizedPath(root.diskRoot))
                            root.setDropTarget(root.diskRoot)
                    }

                    onExited: {
                        if (root.dropTargetPath === normalizedPath(root.diskRoot))
                            root.clearDropTarget(root.diskRoot)
                    }

                    onDropped: (drop) => {
                        const source = drop.source
                        const sourcePath = source && source.path ? source.path : ""
                        if (!sourcePath || sourcePath === normalizedPath(root.diskRoot))
                            return

                        if (root.movePathIntoDirectory(sourcePath, normalizedPath(root.diskRoot))) {
                            drop.acceptProposedAction()
                            root.clearDropTarget()
                        }
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    height: root.dropTargetPath === normalizedPath(root.diskRoot) ? 2 : 0
                    color: Theme.accent
                    visible: height > 0
                }

                Column {
                    id: treeColumn
                    width: treeFlick.width

                    Repeater {
                        model: root.visibleTreeEntries

                        delegate: FileTreeItem {
                            required property var modelData
                            width: treeColumn.width
                            panel: root
                            path: modelData.path
                            name: modelData.name
                            isDirectory: modelData.isDirectory
                            isExpanded: modelData.isExpanded
                            hasChildren: modelData.hasChildren
                            isSearchMatch: modelData.isSearchMatch
                            isCurrentFile: modelData.isCurrentFile
                            isDropTarget: root.dropTargetPath === modelData.path
                            isSelected: root.selectedPath === modelData.path
                            depth: modelData.depth
                            objectName: modelData.path
                            onFileClicked: (path) => {
                                root.setSelectionByPath(path)
                                root.fileClicked(path)
                            }
                            onFolderNavigationRequested: (folderPath) => {
                                root.setSelectionByPath(folderPath)
                                root.toggleFolderExpanded(folderPath)
                            }
                        }
                    }

                    Item {
                        width: treeColumn.width
                        height: root.visibleTreeEntries.length === 0 ? 180 : 0
                        visible: root.visibleTreeEntries.length === 0

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            Label {
                                Layout.fillWidth: true
                                text: root.filterText.trim()
                                      ? "No matches"
                                      : (root.agent && root.agent.workspaceRecentFiles.length > 0
                                         ? "Recent Files"
                                         : "No files in workspace")
                                color: Theme.textMuted
                                font.pixelSize: Theme.fontSm
                            }

                            ListView {
                                Layout.fillWidth: true
                                Layout.fillHeight: false
                                implicitHeight: contentHeight
                                clip: true
                                visible: !root.filterText.trim() && root.agent && root.agent.workspaceRecentFiles.length > 0
                                model: root.agent ? root.agent.workspaceRecentFiles : []

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
                }
            }
        }
    }
}
