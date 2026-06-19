# NeurX-Code Explorer Implementation - Complete Reference

## Architecture Overview

This explorer implementation is based on VSCode's file explorer architecture, adapted for Qt/QML.

### Components

#### 1. **FileTreePanel.qml** (Main Container)
- Root component for the entire file explorer
- Manages navigation, filtering, and file operations
- Handles state persistence for expanded folders
- Provides integrated context menu and file operation dialogs

**Key Features:**
- Multi-level folder navigation with "Go Up" button
- Jump to workspace button
- Expand all / Collapse all controls
- New file / New folder buttons in header
- Search/filter functionality
- Clipboard management (copy/paste)
- Recent files display

#### 2. **FileTreeItem.qml** (Recursive Tree Node)
- Recursive component that displays file/folder tree structure
- Supports nested folder expansion
- Intelligent sorting (folders first, then by name)
- File type icon detection with emoji
- Filter support with real-time matching

**Key Features:**
- Expand/collapse buttons for directories
- File type icons (📁, 📄, 🐍, ⚛️, etc.)
- Recursive nesting for unlimited depth
- Context menu support (right-click)
- Filter highlighting
- Recent files fallback when directory is empty

#### 3. **ExplorerContextMenu.qml** (Right-Click Menu)
- Context menu for file/folder operations
- Based on VSCode's context menu pattern

**Menu Options:**
- New File
- New Folder
- Rename
- Delete
- Copy
- Cut
- Paste
- Copy Path
- Reveal in Finder (macOS)
- Open in Terminal

#### 4. **FileOperationDialog.qml** (Unified Dialog)
- Unified dialog for all file operations
- Supports create, rename, delete modes
- Proper error handling and validation

**Modes:**
- `new-file`: Create new file
- `new-folder`: Create new folder
- `rename`: Rename file/folder
- `delete`: Delete with confirmation

---

## How It Works

### File Tree Rendering
```
FileTreePanel
  └── FileTreeItem (depth=0, dirPath=/workspace)
      ├── Expanded Folder
      │   └── FileTreeItem (depth=1, dirPath=/workspace/folder)
      │       ├── Nested File
      │       └── Nested Folder
      │           └── FileTreeItem (depth=2)
      └── Collapsed Folder
```

### Folder Expansion Logic
1. User clicks expand button on folder
2. `setFolderExpanded()` updates `expandedFolders` state
3. Nested FileTreeItem receives `dirPath` and becomes visible
4. `listDirectoryContents()` is called from C++ backend
5. Items are sorted (directories first, then by name)
6. UI updates with new items

### File Operations Flow
```
User Action (Right-click)
  ↓
ExplorerContextMenu
  ↓
Emit Signal (e.g., createFileRequested)
  ↓
FileTreePanel Handler (onCreateFileRequested)
  ↓
FileOperationDialog.openCreateFile()
  ↓
User Enters Name & Confirms
  ↓
Dialog.onAccepted()
  ↓
agent.createWorkspaceEntry()
  ↓
UI Updates via signal
```

### Copy/Paste/Cut Flow
1. User selects "Copy" from context menu
2. `clipboardContent = { path, mode: "copy" }`
3. User navigates to destination folder
4. User selects "Paste"
5. If mode is "cut": `agent.moveWorkspacePath()`
6. If mode is "copy": `agent.copyWorkspacePath()`
7. `clipboardContent` is cleared (for cut operations)

### Filter/Search Flow
1. User types in filter field
2. `root.filterText` changes
3. `onFilterTextChanged()` triggered
4. `updateSearchExpansion()` finds matching files
5. FileTreeItem filters items in real-time
6. Matching folders auto-expand if they contain matches

---

## State Management

### Persistent State
```javascript
root.expandedPathsByWorkspaceJson  // JSON string of expanded paths per workspace
root.folderExpandedStates           // Current session's expanded folders
```

### Dynamic State
```javascript
root.filterText                     // Current search filter
root.clipboardContent              // Clipboard state for copy/cut/paste
root.diskRoot                       // Currently displayed root folder
```

---

## Icon Support

### File Type Icons (Emoji-based)
```javascript
const iconMap = {
    'js': '📄',   'ts': '📄',   'jsx': '⚛️',   'tsx': '⚛️',
    'py': '🐍',   'cpp': '🔧',  'h': '🔧',     'c': '🔧',
    'java': '☕', 'rs': '🦀',   'go': '🐹',    'rb': '💎',
    'json': '📋', 'yaml': '📋', 'toml': '📋',  'xml': '📋',
    'md': '📝',   'txt': '📝',  'rst': '📝',
    'png': '🖼️', 'jpg': '🖼️', 'jpeg': '🖼️', 'gif': '🖼️', 'svg': '🖼️',
    'css': '🎨',  'scss': '🎨', 'less': '🎨',
    'html': '🌐', 'htm': '🌐',
    'pdf': '📕',  'doc': '📕', 'docx': '📕'
}
```

---

## C++ Backend Requirements

### Required Methods in AgentController:
```cpp
Q_INVOKABLE QVariantList listDirectoryContents(const QString &path);
Q_INVOKABLE bool createWorkspaceEntry(const QString &dirPath, const QString &name, bool isFolder);
Q_INVOKABLE bool renameWorkspacePath(const QString &oldPath, const QString &newName);
Q_INVOKABLE bool deleteWorkspacePath(const QString &path);
Q_INVOKABLE bool moveWorkspacePath(const QString &srcPath, const QString &destPath);
Q_INVOKABLE bool copyWorkspacePath(const QString &srcPath, const QString &destPath);
Q_INVOKABLE QStringList searchWorkspacePaths(const QString &query);
Q_INVOKABLE void copyToClipboard(const QString &text);
Q_INVOKABLE void revealInFinder(const QString &path);  // macOS
Q_INVOKABLE void openInTerminal(const QString &dirPath);
```

### Return Format for listDirectoryContents:
```javascript
[
    {
        name: "filename.txt",
        path: "/full/path/to/filename.txt",
        isDirectory: false,
        size: 1024,
        modified: timestamp
    },
    {
        name: "folder_name",
        path: "/full/path/to/folder_name",
        isDirectory: true,
        size: 0
    }
]
```

---

## Usage Example

### Integrate into Your App

1. **Add to Main QML:**
```qml
import NeurXCode

Item {
    FileTreePanel {
        id: explorer
        anchors.fill: parent
        agent: myAgentController
        
        onFileClicked: (path) => {
            console.log("File clicked:", path)
            // Open file in editor
            editorPanel.openFile(path)
        }
    }
}
```

2. **Connect to Agent:**
```qml
Connections {
    target: explorer
    function onFileClicked(path) {
        myEditorComponent.loadFile(path)
    }
}
```

---

## Keyboard Shortcuts (Future Enhancement)

| Shortcut | Action |
|----------|--------|
| `Cmd+Shift+E` | Focus explorer |
| `Cmd+N` | New file |
| `Cmd+Shift+N` | New folder |
| `Delete` | Delete selected item |
| `Enter` | Rename selected item |
| `Cmd+C` | Copy selected |
| `Cmd+X` | Cut selected |
| `Cmd+V` | Paste |
| `Cmd+F` | Focus search |
| `Escape` | Close menu/dialog |

---

## Performance Considerations

1. **Lazy Loading**: Folders only load contents when expanded
2. **Incremental Updates**: Only re-render changed items
3. **Filter Optimization**: Filter applied before rendering
4. **State Caching**: Expanded state persisted per workspace

---

## Future Enhancements

- [ ] Drag & drop file reordering
- [ ] Multi-select operations
- [ ] File previews
- [ ] Git status indicators
- [ ] Keyboard navigation
- [ ] Custom sorting options
- [ ] File type grouping
- [ ] Breadcrumb navigation
- [ ] Quick preview on hover
- [ ] Search with regex support

---

## Related VSCode Architecture References

- Explorer Service: `src/vs/workbench/contrib/files/browser/explorerService.ts`
- Explorer Model: `src/vs/workbench/contrib/files/common/explorerModel.ts`
- Explorer View: `src/vs/workbench/contrib/files/browser/views/explorerView.ts`
- Explorer Viewer: `src/vs/workbench/contrib/files/browser/views/explorerViewer.ts`
