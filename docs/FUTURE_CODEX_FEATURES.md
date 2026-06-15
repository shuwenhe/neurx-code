# Codex 文件操作功能 - 未来可实现清单

基于对 codex 项目的详细分析，本文档列出了还可以在 neurx-code 中实现的高级文件操作功能。

## 已实现的功能 ✅

### 基础操作 (已完成)
- ✅ read_file - 读取文件内容
- ✅ write_file - 写入文件内容
- ✅ list_directory - 列出目录内容
- ✅ create_file - 创建文件（支持多种模式）
- ✅ delete_file - 删除文件
- ✅ move_file - 移动/重命名文件
- ✅ copy_file - 复制文件/目录

### 高级操作 (刚完成)
- ✅ get_metadata - 获取文件元数据
- ✅ create_directory - 创建目录
- ✅ remove - 增强型删除（递归/强制）
- ✅ canonicalize - 路径规范化
- ✅ join - 路径拼接
- ✅ parent - 获取父目录

---

## 未来可实现的功能 📋

### 1. **文件监视 (Watch/Unwatch)** - 高优先级

**来源**: codex 的 `fs_watch.rs` 和 `fs_processor.rs`

**功能说明**:
- 监听指定路径的文件系统变化
- 实时推送文件变化事件给客户端
- 支持按连接ID管理多个监视任务

**需要实现**:
```cpp
ToolResult opWatch(const QString &callId, const QJsonObject &args);
ToolResult opUnwatch(const QString &callId, const QJsonObject &args);
```

**参数**:
```json
{
  "operation": "watch",
  "path": "src/",
  "watch_id": "watch_123",
  "recursive": true
}
```

**涉及技术**:
- Qt 的 QFileSystemWatcher
- 信号/槽机制
- WebSocket 推送（用于实时通知）
- 事件队列管理

**复杂度**: 🟠 中等 (需要异步事件处理)

---

### 2. **文件搜索 (Search)** - 高优先级

**来源**: codex 的 `file-search/` 模块

**功能说明**:
- 使用模糊匹配进行文件搜索
- 使用 BM25 算法改进搜索相关性
- 并行搜索支持
- 遵守 .gitignore 规则

**需要实现**:
```cpp
ToolResult opSearch(const QString &callId, const QJsonObject &args);
ToolResult opSearchText(const QString &callId, const QJsonObject &args);
```

**参数示例**:
```json
{
  "operation": "search",
  "pattern": "FileSystem*.cpp",
  "path": "src/",
  "recursive": true,
  "max_results": 50
}
```

**返回示例**:
```json
{
  "matches": [
    {
      "path": "src/tools/FileSystemTool.cpp",
      "type": "file",
      "relevance": 0.95
    }
  ],
  "total": 1
}
```

**涉及技术**:
- 文件名匹配算法
- 正则表达式支持
- .gitignore 解析
- 多线程搜索

**复杂度**: 🟠 中等 (需要搜索算法)

---

### 3. **文件权限管理** - 中优先级

**来源**: codex 的 `windows-sandbox-rs/acl.rs` 和通用权限模型

**功能说明**:
- 获取/设置文件权限
- Unix: chmod 权限
- Windows: ACL 管理
- 所有者和组管理

**需要实现**:
```cpp
ToolResult opGetPermissions(const QString &callId, const QJsonObject &args);
ToolResult opSetPermissions(const QString &callId, const QJsonObject &args);
```

**参数示例**:
```json
{
  "operation": "get_permissions",
  "path": "src/main.cpp"
}
```

**返回示例**:
```json
{
  "path": "src/main.cpp",
  "mode": "0644",
  "owner": "feifei",
  "group": "staff",
  "readable": true,
  "writable": true,
  "executable": false,
  "permissions_octal": "644"
}
```

**涉及技术**:
- Unix: stat() 和 chmod()
- Windows: GetFileSecurity() / SetFileSecurity()
- 跨平台权限抽象

**复杂度**: 🟡 中等 (跨平台复杂性)

**平台支持**:
- ✅ macOS/Linux: POSIX 权限
- ⚠️ Windows: 需要 ACL 处理

---

### 4. **符号链接操作** - 中优先级

**来源**: codex 的 `get_metadata` 中的 `is_symlink` 字段

**功能说明**:
- 创建符号链接
- 读取符号链接目标
- 符号链接路径解析

**需要实现**:
```cpp
ToolResult opCreateSymlink(const QString &callId, const QJsonObject &args);
ToolResult opReadSymlink(const QString &callId, const QJsonObject &args);
```

**参数示例**:
```json
{
  "operation": "create_symlink",
  "target": "src/main.cpp",
  "link": "main"
}
```

**涉及技术**:
- Qt 的 QFile::link()
- symlink() 系统调用
- Windows: CreateSymbolicLink()

**复杂度**: 🟡 中等 (平台差异)

---

### 5. **文本搜索与替换** - 中优先级

**功能说明**:
- 在文件中搜索文本
- 支持正则表达式
- 多文件批量替换

**需要实现**:
```cpp
ToolResult opSearchText(const QString &callId, const QJsonObject &args);
ToolResult opReplaceText(const QString &callId, const QJsonObject &args);
```

**参数示例**:
```json
{
  "operation": "search_text",
  "path": "src/",
  "pattern": "TODO.*",
  "regex": true,
  "case_sensitive": false
}
```

**复杂度**: 🟡 中等 (需要正则表达式库)

---

### 6. **文件比较 (Diff)** - 低优先级

**功能说明**:
- 比较两个文件的差异
- 支持二进制和文本文件
- 生成 unified diff 格式

**需要实现**:
```cpp
ToolResult opDiff(const QString &callId, const QJsonObject &args);
```

**参数示例**:
```json
{
  "operation": "diff",
  "path1": "src/old.cpp",
  "path2": "src/new.cpp",
  "format": "unified"
}
```

**复杂度**: 🟡 中等 (需要 diff 算法)

---

### 7. **压缩/解压** - 低优先级

**功能说明**:
- 创建 zip/tar 压缩包
- 解压压缩包
- 递归压缩目录

**参数示例**:
```json
{
  "operation": "compress",
  "path": "build/",
  "format": "zip",
  "destination": "build.zip"
}
```

**涉及库**:
- Qt: QuaZip (Qt 的 zip 库)
- 系统命令: tar, zip

**复杂度**: 🟠 中等 (需要第三方库)

---

### 8. **文件属性扩展** - 低优先级

**功能说明**:
- 读取/写入扩展属性 (xattr)
- 获取文件标签
- 存储自定义元数据

**涉及平台**:
- macOS: xattr 系统
- Linux: extended attributes
- Windows: 备用数据流 (ADS)

**复杂度**: 🔴 高 (平台差异大)

---

## 实现优先级建议

### 第一阶段 (立即实现)
1. **Watch/Unwatch** - 文件变化实时通知很重要
2. **Search** - 在大型代码库中很实用

### 第二阶段 (近期实现)
3. **Permissions** - 安全和权限管理
4. **Symlink** - 符号链接支持
5. **SearchText** - 文本搜索对开发很有帮助

### 第三阶段 (可选实现)
6. **Diff** - 文件比较工具
7. **Compress** - 文件打包
8. **Extended Attributes** - 高级元数据

---

## 与 codex 的特性对标

| 功能 | Codex 实现 | NeurX 建议实现 | 优先级 |
|------|----------|-------------|--------|
| Watch/Unwatch | ✅ fs_watch.rs | 🟡 计划中 | 🟠 高 |
| Search | ✅ file-search/ | 🟡 计划中 | 🟠 高 |
| Permissions | ✅ acl.rs | 🟡 计划中 | 🟡 中 |
| Symlink | ✅ 部分支持 | 🟡 计划中 | 🟡 中 |
| Diff | ❌ 无 | 🟡 计划中 | 🟢 低 |
| Compress | ❌ 无 | 🟡 计划中 | 🟢 低 |

---

## 技术依赖分析

### 需要添加的 Qt 模块
- `QFileSystemWatcher` - 文件监视（已有）
- `QRegularExpression` - 正则表达式搜索
- 可选: `QuaZip` - 压缩支持

### 需要添加的系统库
- Unix: POSIX API (stat, chmod, symlink)
- Windows: Windows API (GetFileSecurity, symlink)

### 需要集成的第三方库
- 可选: 高性能搜索库
- 可选: diff 算法库

---

## 估计工作量

| 功能 | 代码行数 | 时间估计 | 难度 |
|------|--------|---------|------|
| Watch/Unwatch | 150-200 | 3-4h | 中 |
| Search | 200-300 | 4-5h | 中 |
| Permissions | 150-200 | 3-4h | 中 |
| Symlink | 100-150 | 2-3h | 低 |
| SearchText | 150-200 | 3-4h | 中 |
| Diff | 200-300 | 4-6h | 中-高 |
| Compress | 200-300 | 5-7h | 中-高 |

**总计**: ~1200-1700 行代码，约 25-35 小时

---

## 下一步行动

1. ✅ **已完成**: 同步 codex 的 6 个基础高级操作
2. 📋 **建议**: 重点实现 Watch/Unwatch 和 Search
3. 📋 **可选**: 根据实际需求实现权限和符号链接支持

---

**生成日期**: 2026-06-12
**基准项目**: codex @ https://github.com/...