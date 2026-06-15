# 🤖 Agent 请求窗口测试文件写入功能

**目标**: 通过 Agent UI 测试 AgentFileWriterTool 文件写入功能  
**应用状态**: ✅ neurx-codeApp 运行中  
**文件**: hello.cc ✅ 已创建

---

## 🎯 快速开始 (3 步)

### 第 1 步: 打开 Agent 请求窗口
1. 查看 neurx-code 应用窗口
2. 找到右侧的 **Agent 聊天面板** (通常在右侧或底部)
3. 找到输入框，按 `Enter` 或点击发送按钮

### 第 2 步: 复制以下测试提示词之一
参考下面的提示词列表

### 第 3 步: 观察结果
- Agent 应该识别工具需求
- 显示工具调用过程
- 返回成功或失败结果

---

## 📝 即用型测试提示词

### ✅ 测试 1: 基础写入 (最简单)
**复制以下文本到 Agent 输入框：**
```
Write a C++ program that outputs "Hello, World!" to src/hello.cc
```

**预期结果**:
```
✅ File written successfully
Path: src/hello.cc
Size: 97 bytes
```

**验证命令**:
```bash
cat /Users/feifei/agent/neurx-code/src/hello.cc
g++ /Users/feifei/agent/neurx-code/src/hello.cc -o /tmp/test && /tmp/test
```

---

### ✅ 测试 2: 创建新文件 (简单)
**复制以下文本到 Agent 输入框：**
```
Create a file named src/math.cpp with a simple calculator class that has add() and subtract() methods
```

**预期结果**:
```
✅ File created
Path: src/math.cpp
Status: success
```

**验证**:
```bash
cat /Users/feifei/agent/neurx-code/src/math.cpp
```

---

### ✅ 测试 3: 修改现有文件 (中等)
**先执行测试 1，然后输入：**
```
Append a copyright notice to src/hello.cc at the beginning:
// Copyright (C) 2026 NeurX Team
```

**预期结果**:
```
✅ File updated
Mode: prepend
Old size: 97
New size: 157
```

**验证**:
```bash
head -3 /Users/feifei/agent/neurx-code/src/hello.cc
```

---

### ✅ 测试 4: 批量创建文件 (进阶)
**复制以下文本到 Agent 输入框：**
```
Create multiple C++ files in one operation:
1. src/main.cpp - Entry point with main() function
2. src/utils.cpp - Utility functions implementation
3. include/utils.h - Header file with function declarations

Use the agent_file_writer tool with write_batch operation for atomic creation.
```

**预期结果**:
```
✅ Batch write successful
Count: 3 files created
Status: success
```

**验证**:
```bash
ls -la /Users/feifei/agent/neurx-code/src/main.cpp \
        /Users/feifei/agent/neurx-code/src/utils.cpp \
        /Users/feifei/agent/neurx-code/include/utils.h
```

---

### ✅ 测试 5: 创建项目结构 (高级)
**复制以下文本到 Agent 输入框：**
```
Create a complete C++ project structure with the following directories and files:
- src/main.cpp (entry point)
- src/handler.cpp (request handler)
- include/handler.h (header)
- Makefile (build configuration)
- README.md (documentation)
- .gitignore (git ignore file)

Create all these in one operation using the write_batch mode.
```

**预期结果**:
```
✅ Project structure created
6 files created
Status: success
```

---

### ✅ 测试 6: 更新现有文件的特定行 (高级)
**复制以下文本到 Agent 输入框：**
```
Insert a function declaration into src/math.cpp at line 2:
double divide(double a, double b);

Use the update_file operation with insert mode.
```

**预期结果**:
```
✅ File updated
Mode: insert
New size: increased
```

---

## 📊 工具操作对照表

| 操作 | 触发关键词 | 示例 |
|------|-----------|------|
| **write_single** | write, create, make | "Create a file src/test.cpp" |
| **write_batch** | multiple, batch, project | "Create multiple files" |
| **update_file** | append, add, insert, prepend | "Add content to file" |
| **write_template** | generate, template, class | "Generate a C++ class" |
| **create_structure** | structure, directory | "Create directory structure" |

---

## 🔍 如何判断成功

### ✅ 成功标志
- [ ] Agent 输出包含 "agent_file_writer" 或 "write_single" 等工具名称
- [ ] 显示 JSON 格式的工具调用参数
- [ ] 返回结果中包含 `"status": "success"` 或类似
- [ ] 没有出现红色错误信息

### 📊 成功的 Agent 响应示例
```
Tool: agent_file_writer
Operation: write_single
Path: src/hello.cc
Status: ✅ success

Result:
{
  "path": "src/hello.cc",
  "size": 97,
  "backup": "src/hello.cc.backup.20260611_174000"
}
```

---

## 🧪 完整测试流程示例

### 场景：创建一个简单的 C++ 项目

**Step 1: 创建 main.cpp**
```
Agent输入: Write a C++ main.cpp that calls a greet() function

观察:
✅ Tool: agent_file_writer
✅ Operation: write_single
✅ File created: src/main.cpp
```

**Step 2: 创建 utils.cpp**
```
Agent输入: Create a utils.cpp file with a greet() function implementation

观察:
✅ File created: src/utils.cpp
```

**Step 3: 更新 main.cpp**
```
Agent输入: Add #include "utils.h" at the top of src/main.cpp

观察:
✅ File updated: src/main.cpp
✅ Mode: prepend
```

**Step 4: 验证**
```bash
cd /Users/feifei/agent/neurx-code
g++ -I include src/main.cpp src/utils.cpp -o test_app
./test_app
```

---

## 📋 测试检查清单

完成每个测试后，检查：

- [ ] Agent 识别了文件操作需求
- [ ] Agent 调用了 agent_file_writer 工具
- [ ] 工具返回了成功状态
- [ ] 文件在预期位置被创建
- [ ] 文件内容与预期一致
- [ ] 如有备份，备份文件已创建
- [ ] 没有权限或路径错误

---

## 🔧 常见提示词模板

### 模板 1: 创建单个文件
```
Create a file [path] with [description]
```

**示例**:
```
Create a file src/config.cpp with a Configuration class
```

### 模板 2: 修改文件
```
[append/prepend/insert] [content] to [path]
```

**示例**:
```
Prepend a copyright notice to src/config.cpp
```

### 模板 3: 批量创建
```
Create multiple files:
1. [path1] - [description1]
2. [path2] - [description2]
...
```

**示例**:
```
Create multiple files:
1. src/app.cpp - Application main
2. src/server.cpp - Server implementation
3. include/server.h - Server header
```

---

## 🐛 故障排查

### 问题 1: Agent 没有调用工具
**原因**：提示词中没有明确的文件操作关键词

**解决**：
- 使用 "write", "create", "generate" 等关键词
- 明确指定文件路径
- 提供具体的代码内容

**改进例**:
```
❌ 不好: "写个C++程序"
✅ 好: "Create a C++ program and save to src/app.cpp with the following content: ..."
```

### 问题 2: 文件未被创建
**检查**：
```bash
# 验证目录
ls -la /Users/feifei/agent/neurx-code/src/

# 检查权限
ls -ld /Users/feifei/agent/neurx-code/src

# 查看备份文件
find /Users/feifei/agent/neurx-code -name "*.backup.*"
```

### 问题 3: Agent 返回错误
**常见错误**:
- "Path traversal detected" - 使用了 `../` 或绝对路径
- "File not found" - 在更新文件时文件不存在
- "Content too large" - 文件超过 10MB

**解决**:
- 使用相对于工作区的路径 (如 `src/app.cpp`)
- 先创建文件再更新
- 检查文件内容大小

---

## ✨ 高级用法

### 用 Agent 生成代码
```
Agent输入: Generate a C++ class for managing a database connection with methods: 
connect(), disconnect(), query(). Save to include/Database.h and src/Database.cpp
```

### 用 Agent 创建整个项目
```
Agent输入: Create a complete CMake-based C++ project structure with:
- src/ directory with main.cpp
- include/ directory with headers
- CMakeLists.txt
- README.md
```

### 用 Agent 进行代码重构
```
Agent输入: Read src/old_code.cpp, refactor it for better performance, 
and save the refactored version to src/new_code.cpp
```

---

## 🎯 验证工具是否正确工作

### 方式 1: 直接查看文件
```bash
# 查看最近创建的文件
ls -lt /Users/feifei/agent/neurx-code/src/ | head -5

# 查看文件内容
cat /Users/feifei/agent/neurx-code/src/hello.cc

# 检查文件大小
du -h /Users/feifei/agent/neurx-code/src/hello.cc
```

### 方式 2: 编译测试
```bash
cd /Users/feifei/agent/neurx-code
g++ src/hello.cc -o /tmp/test -std=c++17 && echo "✅ Compile OK" && /tmp/test
```

### 方式 3: 查看 Agent 日志
在 Agent 界面查看：
- 工具调用记录
- 执行状态
- 返回结果

---

## 🎉 推荐的测试顺序

1. **第 1 天**：执行测试 1 (基础写入)
2. **第 2 天**：执行测试 2 和 3 (创建和修改)
3. **第 3 天**：执行测试 4 (批量创建)
4. **第 4 天**：执行测试 5 和 6 (高级功能)

---

## 📚 相关文档

- [完整实现指南](HELLO_CC_IMPLEMENTATION.md)
- [Agent 工具测试指南](AGENT_TOOL_TESTING_GUIDE.md)
- [快速测试指南](QUICK_TEST.md)
- [完成总结](HELLO_CC_COMPLETION_SUMMARY.md)

---

## 🚀 现在就开始！

1. 🔍 打开 neurx-code 应用
2. 👁️ 找到 Agent 输入框
3. 📋 复制上面的测试提示词之一
4. ⌨️ 按 Enter 发送
5. ✨ 观察 Agent 执行工具
6. ✅ 验证文件创建成功

**准备好了吗？开始测试吧！** 🎊
