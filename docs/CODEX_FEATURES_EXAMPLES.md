# NeurX Code - Codex 新增文件操作功能使用示例

本文档展示如何使用从 codex 同步来的新文件系统操作功能。

## 1. 获取文件元数据 (get_metadata)

获取文件或目录的完整属性信息。

### 请求示例
```json
{
  "operation": "get_metadata",
  "path": "src/main.cpp"
}
```

### 返回示例
```json
{
  "path": "src/main.cpp",
  "absolute_path": "/Users/feifei/agent/neurx-code/src/main.cpp",
  "is_file": true,
  "is_directory": false,
  "is_symlink": false,
  "size": 2048,
  "created_time_ms": 1718112000000,
  "modified_time_ms": 1718198400000,
  "accessed_time_ms": 1718198400000,
  "readable": true,
  "writable": true,
  "executable": false,
  "hidden": false
}
```

### 目录元数据示例
```json
{
  "operation": "get_metadata",
  "path": "src"
}
```

返回的关键字段：
- `is_directory`: true
- `entry_count`: 15 (目录内文件/文件夹数量)

---

## 2. 创建目录 (create_directory)

创建单个或多级目录。

### 创建单级目录
```json
{
  "operation": "create_directory",
  "path": "build/output",
  "recursive": false
}
```

### 创建多级目录（推荐）
```json
{
  "operation": "create_directory",
  "path": "build/debug/logs/2026-06",
  "recursive": true
}
```

### 返回示例
```
Directory created: build/debug/logs/2026-06
Checkpoint: checkpoint-20260612-143022
```

---

## 3. 增强的删除操作 (remove)

删除文件或目录，支持递归和强制模式。

### 删除单个文件
```json
{
  "operation": "remove",
  "path": "build/temp.txt"
}
```

### 删除目录及其内容
```json
{
  "operation": "remove",
  "path": "build/output",
  "recursive": true
}
```

### 强制删除（不检查存在性）
```json
{
  "operation": "remove",
  "path": "build/maybe-exists.txt",
  "force": true
}
```

### 返回示例
```
Removed: build/output
Checkpoint: checkpoint-20260612-143045
```

---

## 4. 路径规范化 (canonicalize)

解析和规范化路径，消除 `..` 和 `.` 等相对引用。

### 请求示例
```json
{
  "operation": "canonicalize",
  "path": "src/../include/./header.h"
}
```

### 返回示例
```json
{
  "original": "src/../include/./header.h",
  "absolute": "/Users/feifei/agent/neurx-code/include/header.h",
  "relative": "include/header.h",
  "canonical": "/Users/feifei/agent/neurx-code/include/header.h",
  "exists": true
}
```

### 用例：路径去重
在进行文件操作前，使用此功能确保获取规范路径：
```json
{
  "operation": "canonicalize",
  "path": "./src/tools/../FileSystemTool.cpp"
}
```

---

## 5. 路径拼接 (join)

智能拼接多个路径片段，返回规范化的绝对路径。

### 基本拼接
```json
{
  "operation": "join",
  "paths": ["src", "tools", "FileSystemTool.cpp"]
}
```

### 返回示例
```json
{
  "joined": "/Users/feifei/agent/neurx-code/src/tools/FileSystemTool.cpp",
  "relative": "src/tools/FileSystemTool.cpp",
  "exists": true
}
```

### 实际用例：动态构建文件路径
```json
{
  "operation": "join",
  "paths": ["build", "debug", "config.json"]
}
```

返回的路径适合直接用于后续操作（write_file, read_file 等）。

---

## 6. 获取父目录 (parent)

获取文件或目录的父目录信息。

### 请求示例
```json
{
  "operation": "parent",
  "path": "src/tools/FileSystemTool.cpp"
}
```

### 返回示例
```json
{
  "original": "src/tools/FileSystemTool.cpp",
  "parent": "/Users/feifei/agent/neurx-code/src/tools",
  "relative": "src/tools",
  "exists": true
}
```

### 获取顶级目录
```json
{
  "operation": "parent",
  "path": "README.md"
}
```

返回工作区根目录。

---

## 完整工作流示例

### 场景：创建新的项目结构

```json
// 1. 创建目录结构
{
  "operation": "create_directory",
  "path": "projects/my-app/src/components",
  "recursive": true
}

// 2. 验证创建成功
{
  "operation": "get_metadata",
  "path": "projects/my-app/src"
}

// 3. 获取相对路径
{
  "operation": "canonicalize",
  "path": "projects/my-app/src"
}

// 4. 创建文件
{
  "operation": "create_file",
  "path": "projects/my-app/src/main.cpp",
  "content": "#include <iostream>\n\nint main() {\n    return 0;\n}\n"
}
```

### 场景：安全的文件移动和清理

```json
// 1. 检查源文件
{
  "operation": "get_metadata",
  "path": "src/old_tool.cpp"
}

// 2. 规范化目标路径
{
  "operation": "canonicalize",
  "path": "archived/old_tool.cpp"
}

// 3. 移动文件
{
  "operation": "move_file",
  "path": "src/old_tool.cpp",
  "destination": "archived/old_tool.cpp"
}

// 4. 清理临时目录
{
  "operation": "remove",
  "path": "build/temp",
  "recursive": true,
  "force": true
}
```

---

## 参数参考表

| 操作 | 必需参数 | 可选参数 | 返回格式 |
|------|--------|--------|--------|
| get_metadata | path | - | JSON |
| create_directory | path | recursive | 文本 |
| remove | path | recursive, force | 文本 |
| canonicalize | path | - | JSON |
| join | paths[] | - | JSON |
| parent | path | - | JSON |

---

## 错误处理

所有操作返回统一的 ToolResult 格式：

### 成功响应
```cpp
{
  callId: "xxx",
  name: "file_system",
  isError: false,
  result: "..."
}
```

### 错误响应
```cpp
{
  callId: "xxx",
  name: "file_system",
  isError: true,
  result: "Path traversal denied." // 或其他错误信息
}
```

### 常见错误
| 错误 | 原因 | 解决方案 |
|------|------|--------|
| "Path traversal denied." | 试图访问工作区外的路径 | 使用相对路径 |
| "Sandbox policy denied access." | 沙箱权限限制 | 检查 SandboxManager 配置 |
| "Path does not exist." | 操作对象不存在 | 使用 create_* 操作创建 |

---

## 与 codex 的兼容性

这些新操作与 codex 的同名操作保持接口兼容，但实现基于 Qt 框架优化：

| 特性 | NeurX (Qt) | Codex (Rust) |
|------|-----------|-------------|
| 路径沙箱 | ✅ | ✅ |
| 检查点追踪 | ✅ | ✅ |
| 元数据完整性 | ✅ | ✅ |
| 多平台支持 | Windows/Mac/Linux | 多平台 |

---

## 更新日期
- **2026-06-12**: 初始版本，同步 codex 的 6 个文件操作功能
