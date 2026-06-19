# NeurX-Code Explorer - Quick Start Guide

## What's New

Your neurx-code explorer now has VSCode-like features:

### 🌳 Folder Navigation
- Click expand (▶) / collapse (▼) buttons to show/hide folder contents
- Click up (↑) button to go to parent folder
- Click home (⌂) button to jump to workspace root
- Path shows current location at the top

### 📁 File Management
- **Right-click** any file or folder for context menu
- **New File**: Right-click → "New File" or use toolbar (+) button
- **New Folder**: Right-click → "New Folder" or use toolbar (📁+) button
- **Rename**: Right-click → "Rename", enter new name
- **Delete**: Right-click → "Delete", confirm deletion

### 📋 Copy & Paste
- **Copy**: Right-click file/folder → "Copy"
- **Cut**: Right-click → "Cut" (for moving)
- **Paste**: Right-click in destination folder → "Paste"
- **Copy Path**: Right-click → "Copy Path" (copies full path to clipboard)

### 🔍 Search & Filter
- Type in filter box to search files
- Shows only matching files/folders
- Matching folders auto-expand to show results
- Results are case-insensitive

### 🎯 Quick Actions
- **Expand All** (▼▼ button): Opens all nested folders
- **Collapse All** (▶▶ button): Closes all folders
- **New File** (+): Create file in current directory
- **New Folder** (📁+): Create folder in current directory

### 🎨 Visual Features
- **Folder Icons**: 📁 for folders
- **File Type Icons**: 
  - 📄 for generic files
  - 🐍 for Python
  - ⚛️ for React/JSX
  - 🔧 for C/C++/Header files
  - 🌐 for HTML
  - 🎨 for CSS/SCSS
  - 📝 for text/markdown
  - 🖼️ for images
  - And 15+ more!
- **Recent Files**: Shows when folder is empty

### 💾 Smart Persistence
- Expanded/collapsed state saved per workspace
- Remembered when you switch workspaces
- Filter state resets when changing directories

---

## Keyboard & Mouse Usage

### Mouse
- **Left-click** ▶/▼: Expand/collapse folder
- **Left-click** file: Open file in editor
- **Right-click**: Context menu (file operations)
- **Hover**: Shows tooltips on buttons

### Keyboard
- **Type in filter**: Real-time search/filter
- **Enter** in dialog: Confirm operation
- **Escape**: Close dialogs/menus
- **Tab**: Navigate between fields

---

## Comparison with VSCode

| Feature | NeurX-Code | VSCode |
|---------|-----------|--------|
| Folder expansion | ✅ | ✅ |
| Recursive tree | ✅ | ✅ |
| File type icons | ✅ (emoji) | ✅ (font) |
| Create file/folder | ✅ | ✅ |
| Rename | ✅ | ✅ |
| Delete | ✅ | ✅ |
| Copy/Paste | ✅ | ✅ |
| Context menu | ✅ | ✅ |
| Search/Filter | ✅ | ✅ |
| Expand all | ✅ | ✅ |
| Collapse all | ✅ | ✅ |
| State persistence | ✅ | ✅ |
| Drag & drop | ⏳ | ✅ |
| Git integration | ⏳ | ✅ |
| File preview | ⏳ | ✅ |
| Multi-select | ⏳ | ✅ |

---

## Common Tasks

### Create a New File
1. Navigate to desired folder
2. Click **+** button in toolbar, OR
3. Right-click in folder → "New File"
4. Type filename (e.g., "myfile.js")
5. Press Enter or click "Create"

### Organize Files
1. Right-click file → "Cut"
2. Navigate to destination folder
3. Right-click in folder → "Paste"

### Rename Multiple Files
1. Right-click file → "Rename"
2. Change name and confirm
3. Repeat for next file

### Find Files Quickly
1. Type in filter box (top of explorer)
2. Only matching files shown
3. Folders with matches auto-expand
4. Clear filter to see all files again

### Expand/Collapse
- Click **▶** to expand a folder
- Click **▼** to collapse a folder
- Click **▼▼** button to expand all folders at once
- Click **▶▶** button to collapse all at once

---

## Tips & Tricks

✨ **Filter Tips**
- Search is case-insensitive
- Type partial name to find files
- Filter auto-expands matching folders
- Clear to see full structure again

✨ **Navigation Tips**
- Click home (⌂) to quickly return to workspace root
- Use up (↑) button to browse parent folders
- Path label shows where you are

✨ **Operation Tips**
- Recent files shown when folder is empty
- All operations are undoable (via agent)
- Dialogs show which item you're operating on
- Confirm deletions to avoid accidents

✨ **Speed Tips**
- Toolbar buttons quick access for common tasks
- Right-click menus context-aware
- Expand only needed folders for performance
- Filter reduces visible items for faster scrolling

---

## Architecture

The explorer consists of:
- **FileTreePanel**: Main container with header and controls
- **FileTreeItem**: Recursive tree nodes for folders
- **ExplorerContextMenu**: Right-click context menu
- **FileOperationDialog**: File creation/rename/delete dialog

All components follow VSCode's explorer architecture patterns.

---

## Troubleshooting

**Q: Filter doesn't clear  
A:** Click the X button or clear the filter field

**Q: Items not showing after delete  
A:** Refresh by clicking parent folder expand/collapse

**Q: Can't see deeply nested folders  
A:** Use "Expand All" button to open all levels

**Q: Recent files not showing  
A:** Recent files only shown when folder is empty

---

For more technical details, see: `EXPLORER_IMPLEMENTATION.md`
