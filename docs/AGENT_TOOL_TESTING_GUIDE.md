# 🤖 通过 Agent 使用 AgentFileWriterTool 测试

**状态**: ✅ **已准备好**  
**应用**: neurx-codeApp 运行中 (PID: 90709)  
**文件**: src/hello.cc ✅ 已创建  

---

## 🎯 测试步骤

### 第 1 步: 打开 Agent 输入窗口
在 neurx-code 应用中，找到 Agent 聊天框（通常在右侧面板）

### 第 2 步: 复制以下提示词之一

#### 🔹 提示词 A: 基础 - 验证文件写入
```
Write a C++ program that outputs "Hello, World!" to src/hello.cc
```

**预期**: 
- Agent 识别文件写入任务
- 调用 agent_file_writer 工具
- 文件被创建或更新
- 显示成功消息

---

#### 🔹 提示词 B: 进阶 - 修改现有文件
```
Append a copyright notice to src/hello.cc at the beginning:
// Copyright (C) 2026 NeurX Team
// All rights reserved.
```

**预期**:
- Agent 调用 update_file 操作
- 模式: prepend (前置)
- 版权信息添加到文件顶部
- 原始内容保留

---

#### 🔹 提示词 C: 完整 - 创建完整项目结构
```
Create a complete C++ project with the following structure:
1. src/main.cpp - Main program file with Hello World
2. include/header.h - Header file with a greeting function declaration
3. Makefile - Build configuration
4. README.md - Project documentation

Use agent_file_writer tool with write_batch operation.
```

**预期**:
- Agent 创建多个文件
- 所有文件一次性创建成功
- 如果失败，全部回滚

---

#### 🔹 提示词 D: 模板 - 从模板生成
```
Generate a C++ class definition template in src/MyClass.h
The class should have:
- A constructor
- A destructor
- A method called greet() that returns a string
- Private member variable m_name
```

**预期**:
- Agent 使用模板生成代码
- 自动格式化和命名

---

#### 🔹 提示词 E: 创建目录结构
```
Create the following directory structure for a C++ project:
- src/
- include/
- build/
- tests/
- docs/

Then create a basic CMakeLists.txt in the root
```

**预期**:
- 创建目录结构
- 生成构建配置文件

---

## 📊 工具操作矩阵

### write_single (单文件写入)
**用途**: 创建或覆盖单个文件  
**触发词**: create, write, make file, generate  
**示例**:
```
Create a file src/test.cpp with the following content:
[代码内容]
```

### write_batch (批量写入)
**用途**: 原子性创建多个文件  
**触发词**: multiple files, project structure, setup project  
**示例**:
```
Create these files in one operation:
1. src/a.cpp
2. src/b.cpp
3. include/h.h
```

### update_file (文件更新)
**用途**: 修改现有文件内容  
**操作模式**:
- `append` - 追加到末尾
- `prepend` - 添加到开头
- `insert` - 在指定行插入
- `overwrite` - 完全覆盖

**触发词**: append, add, prepend, insert, replace, modify  
**示例**:
```
Append the following code to src/hello.cc:
[代码内容]
```

### write_template (模板生成)
**用途**: 使用模板生成代码  
**触发词**: template, generate, class, struct, function  
**示例**:
```
Generate a C++ class template with:
- Name: Logger
- Methods: log(), setLevel()
```

### create_structure (目录结构)
**用途**: 创建目录树  
**触发词**: structure, directory, mkdir, setup folders  
**示例**:
```
Create this structure:
project/
  src/
  include/
  build/
  tests/
```

---

## 📋 验证检查清单

### ✅ Agent 识别工具
```
Agent 消息中应该包含:
"I'll use the agent_file_writer tool to..."
或
"Using write_single operation..."
或
类似的工具识别消息
```

### ✅ 工具调用成功
```
返回结果中应该包含:
{
  "status": "success",
  "path": "src/hello.cc",
  "size": 97,
  ...
}
```

### ✅ 文件已创建
验证命令:
```bash
# 检查文件存在
ls -lh /Users/feifei/agent/neurx-code/src/hello.cc

# 查看文件内容
cat /Users/feifei/agent/neurx-code/src/hello.cc

# 编译文件
cd /Users/feifei/agent/neurx-code
g++ src/hello.cc -o /tmp/test_hello && /tmp/test_hello
```

---

## 🔄 工作流示例

### 完整交互流程

**Step 1**: 用户输入
```
User: Write a C++ "Hello, World!" program to src/hello.cc
```

**Step 2**: Agent 分析
```
Agent: I'll create a C++ Hello World program at src/hello.cc.
I'll use the agent_file_writer tool with write_single operation.
```

**Step 3**: 工具调用
```json
Tool Call: agent_file_writer
Operation: write_single
Path: src/hello.cc
Content: "#include <iostream>\n..."
```

**Step 4**: 工具执行
```
Tool Result:
✅ Success
Path: src/hello.cc
Size: 97 bytes
Created at: /Users/feifei/agent/neurx-code/src/hello.cc
```

**Step 5**: Agent 确认
```
Agent: ✅ Done! I've created src/hello.cc with a C++ 
Hello World program. The file is ready to compile and run.
```

**Step 6**: 用户验证
```bash
g++ /Users/feifei/agent/neurx-code/src/hello.cc -o /tmp/hello
/tmp/hello
# 输出: Hello, World!
```

---

## 🧪 测试场景集合

### 📌 场景 1: 基础写入测试 ⭐
**难度**: 简单  
**时间**: < 1 分钟  
**提示词**:
```
Write a simple C++ program to src/test1.cc that prints "Test 1"
```

**验证**:
```bash
cd /Users/feifei/agent/neurx-code
g++ src/test1.cc -o /tmp/test1 && /tmp/test1
```

---

### 📌 场景 2: 文件更新测试 ⭐⭐
**难度**: 中等  
**时间**: 2-3 分钟  
**提示词**:
```
Add a comment at the top of src/hello.cc:
// Program: Hello World
// Author: NeurX Agent
// Purpose: Demonstration
```

**验证**:
```bash
head -3 /Users/feifei/agent/neurx-code/src/hello.cc
```

---

### 📌 场景 3: 批量创建测试 ⭐⭐⭐
**难度**: 高  
**时间**: 3-5 分钟  
**提示词**:
```
Create a C++ project structure with:
1. src/main.cpp - Entry point
2. src/utils.cpp - Utility functions
3. include/utils.h - Header file
4. Makefile - Build script
5. README.md - Documentation

Create all files in one operation (use write_batch).
```

**验证**:
```bash
cd /Users/feifei/agent/neurx-code
ls -la src/main.cpp src/utils.cpp include/utils.h Makefile README.md
```

---

### 📌 场景 4: 智能代码生成 ⭐⭐⭐
**难度**: 高  
**时间**: 5-10 分钟  
**提示词**:
```
Generate a C++ class for managing a simple task list:
- Class name: TaskManager
- Private members: tasks (vector), capacity (int)
- Public methods: addTask(), removeTask(), getTasks(), print()
- The class should be saved in include/TaskManager.h
- Also create src/TaskManager.cpp with method implementations
```

**验证**:
```bash
cd /Users/feifei/agent/neurx-code
g++ -I include src/TaskManager.cpp -c -o /tmp/TaskManager.o
```

---

## 💡 提示和技巧

### ✅ 有效的提示词特征
- ✅ 明确指定文件路径 (相对于工作区)
- ✅ 清晰描述文件内容或目的
- ✅ 使用 "write", "create", "generate" 等关键词
- ✅ 提供具体的代码或结构
- ✅ 指定操作类型 (单个、批量、更新等)

### ❌ 无效的提示词特征
- ❌ 含糊不清的文件名
- ❌ 绝对路径 (如 /Users/...) 
- ❌ 没有明确的内容描述
- ❌ 混乱的文件结构
- ❌ 超大的代码块

### 🎯 优化技巧
1. **分步操作** - 先创建简单文件，再创建复杂项目
2. **明确目的** - 告诉 Agent 文件的用途
3. **参考示例** - 提供代码示例或模板
4. **验证结果** - 每步完成后验证文件
5. **错误处理** - 如果失败，检查路径或权限

---

## 🔗 快速命令

### 查看文件
```bash
cat /Users/feifei/agent/neurx-code/src/hello.cc
```

### 编译测试
```bash
cd /Users/feifei/agent/neurx-code
g++ src/hello.cc -o /tmp/test && /tmp/test
```

### 列出所有创建的文件
```bash
find /Users/feifei/agent/neurx-code/src -type f -name "*.cc" -o -name "*.cpp" -o -name "*.h"
```

### 查看文件备份
```bash
find /Users/feifei/agent/neurx-code -name "*.backup.*"
```

### 清理测试文件
```bash
rm -f /Users/feifei/agent/neurx-code/src/test*.cc
rm -f /Users/feifei/agent/neurx-code/src/test*.cpp
```

---

## 📞 故障排查

### 问题 1: Agent 没有调用文件写入工具
**解决方案**:
- 确保提示词中包含 "write", "create" 等关键词
- 提供明确的文件路径
- 使用完整的代码示例

### 问题 2: 文件创建失败
**检查**:
```bash
# 检查目录权限
ls -ld /Users/feifei/agent/neurx-code/src

# 检查磁盘空间
df -h /Users/feifei

# 检查文件是否已存在
ls -la /Users/feifei/agent/neurx-code/src/hello.cc
```

### 问题 3: 编译失败
**检查**:
```bash
# 验证 C++ 代码语法
g++ -fsyntax-only /Users/feifei/agent/neurx-code/src/hello.cc

# 查看详细错误
g++ -v src/hello.cc
```

---

## 🎉 成功标志

当看到以下迹象时，说明功能实现成功：

✅ Agent 识别了文件写入任务  
✅ Agent 调用了 agent_file_writer 工具  
✅ 工具返回 "success" 状态  
✅ 文件被创建到指定位置  
✅ 文件内容与预期一致  
✅ 文件可以被编译  
✅ 程序可以正常运行  

---

**准备好了吗？** 🚀

现在您可以：
1. 打开 neurx-code 应用
2. 在 Agent 输入框中复制上述任一提示词
3. 观察 Agent 调用工具并创建文件
4. 验证文件创建成功

**祝您使用愉快！**
