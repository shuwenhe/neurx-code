# 🎯 Agent 文件写入工具 - 快速测试参考卡

## 📌 快速开始 (30 秒)

**打开 neurx-code → 找到 Agent 输入框 → 复制以下文本之一 → 按 Enter**

---

## 🧪 6 个现成的测试提示词

### 1️⃣ **最简单** - 创建 Hello World
```
Write a C++ program that outputs "Hello, World!" to src/hello.cc
```
✅ **预期**: 文件创建成功，97 字节
🔍 **验证**: `cat /Users/feifei/agent/neurx-code/src/hello.cc`

---

### 2️⃣ **简单** - 创建新文件
```
Create a C++ calculator class in src/calc.cpp with add() and subtract() methods
```
✅ **预期**: 文件创建，包含类定义
🔍 **验证**: `cat /Users/feifei/agent/neurx-code/src/calc.cpp`

---

### 3️⃣ **简单** - 修改文件
```
Add this line at the top of src/hello.cc:
// Program: Hello World
// Date: 2026-06-11
```
✅ **预期**: 文件更新，行数增加
🔍 **验证**: `head -2 /Users/feifei/agent/neurx-code/src/hello.cc`

---

### 4️⃣ **进阶** - 批量创建
```
Create three C++ files in one operation:
1. src/main.cpp - with main() function
2. src/utils.cpp - with utility functions
3. include/utils.h - with function declarations
```
✅ **预期**: 3 个文件一次性创建
🔍 **验证**: `ls -la /Users/feifei/agent/neurx-code/src/main.cpp /Users/feifei/agent/neurx-code/src/utils.cpp /Users/feifei/agent/neurx-code/include/utils.h`

---

### 5️⃣ **进阶** - 完整项目
```
Create a complete project structure with:
- src/main.cpp
- src/server.cpp
- include/server.h
- Makefile
- README.md

Create all files in one batch operation.
```
✅ **预期**: 5 个文件创建
🔍 **验证**: `find /Users/feifei/agent/neurx-code -name "main.cpp" -o -name "server.cpp" -o -name "server.h" -o -name "Makefile" -o -name "README.md"`

---

### 6️⃣ **高级** - 代码生成
```
Generate a C++ logger class with these features:
- Constructor that takes a log level
- Methods: info(), warning(), error()
- Member: log file path
Save to include/Logger.h and src/Logger.cpp
```
✅ **预期**: 生成完整的头文件和实现文件
🔍 **验证**: `cat /Users/feifei/agent/neurx-code/include/Logger.h`

---

## 📊 验证快速指南

| 操作 | 验证命令 |
|------|--------|
| 查看文件 | `cat /Users/feifei/agent/neurx-code/src/hello.cc` |
| 编译测试 | `g++ /Users/feifei/agent/neurx-code/src/hello.cc -o /tmp/test && /tmp/test` |
| 列出所有文件 | `find /Users/feifei/agent/neurx-code/src -type f` |
| 查看备份 | `find /Users/feifei/agent/neurx-code -name "*.backup.*"` |
| 文件统计 | `ls -lh /Users/feifei/agent/neurx-code/src/` |

---

## ✨ 成功标志

看到这些说明成功了：
- ✅ Agent 提到 "agent_file_writer" 或 "write_single"
- ✅ 返回 `"status": "success"`
- ✅ 显示文件路径和大小
- ✅ 没有红色错误信息

---

## ❌ 常见问题排查

| 问题 | 解决方案 |
|------|--------|
| 工具未调用 | 使用 "create", "write", "generate" 等关键词 |
| 文件未创建 | 检查路径是否相对路径，不要用 `/Users/...` |
| 编译失败 | 确保 C++ 代码语法正确 |
| 权限错误 | 目录应该已存在：`/Users/feifei/agent/neurx-code/src/` |

---

## 🎯 推荐测试流程

```
Day 1: 测试 1️⃣ (创建 hello.cc)
  ↓
Day 2: 测试 2️⃣ + 3️⃣ (创建 + 修改)
  ↓
Day 3: 测试 4️⃣ (批量创建)
  ↓
Day 4: 测试 5️⃣ + 6️⃣ (高级功能)
```

---

## 📱 格式提示

### ✅ 好的提示词格式
```
Create/Write/Generate [description] to/in [path]
...
[具体内容或要求]
```

### ❌ 避免的格式
```
❌ "写个文件" (太模糊)
❌ "/Users/.../file.cpp" (用绝对路径)
❌ "修改 ../file.cpp" (不要用 ..)
```

---

## 🔗 完整文档位置

- [完整实现指南](HELLO_CC_IMPLEMENTATION.md)
- [详细测试指南](AGENT_REQUEST_WINDOW_TESTING.md) ← 你在这里
- [Agent 工具测试](AGENT_TOOL_TESTING_GUIDE.md)
- [快速测试](QUICK_TEST.md)

---

## 🚀 立即开始

1. 打开 neurx-code 应用（应该已运行）
2. 找到 Agent 聊天窗口（右侧）
3. 在输入框粘贴上面的 **任意一个测试提示词**
4. 按 Enter 或点击发送
5. 观察 Agent 工作...
6. ✅ 验证文件创建成功

**就这么简单！** 🎉
