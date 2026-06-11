# 🧪 Agent File Writer Tool - 测试指南

## ✅ 集成完成

**AgentFileWriterTool** 已成功集成到 neurx-code 中：

- ✅ 源文件添加到编译
- ✅ 工具在 AgentController 中注册
- ✅ 项目编译成功
- ✅ 应用程序运行中

---

## 📋 测试场景：在 hello.cc 中写入 C++ "Hello,World" 代码

### 场景 1: 基本写入测试

**目标**: 使用 `agent_file_writer` 工具在 `src/hello.cc` 中创建一个简单的 C++ 程序

**Agent 提示词**:
```
Write a C++ "Hello, World!" program to src/hello.cc using the agent_file_writer tool.
The file should contain:
1. Standard headers (iostream)
2. Main function that prints "Hello, World!"
3. Return 0

Include proper C++ formatting and comments.
```

**预期工具调用**:
```json
{
  "tool": "agent_file_writer",
  "operation": "write_single",
  "path": "src/hello.cc",
  "content": "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << std::endl;\n    return 0;\n}\n",
  "create_dirs": true,
  "backup": true,
  "validate": true
}
```

**验证结果**:
- ✅ 文件 `/Users/feifei/agent/neurx-code/src/hello.cc` 应该被创建
- ✅ 文件内容应该包含 `#include <iostream>` 和 `std::cout << "Hello, World!"`
- ✅ 返回结果应该包含 `"status": "success"`

---

## 📖 测试步骤

### 步骤 1: 打开 Agent 输入窗口

在 neurx-code 应用中找到 Agent 输入窗口（通常在底部或侧边栏）

### 步骤 2: 输入测试提示词

复制下列提示词之一并粘贴到 Agent 输入窗口：

#### 提示词 A - 基础版
```
Please write a C++ "Hello, World!" program to src/hello.cc
```

#### 提示词 B - 详细版
```
Use the agent_file_writer tool to create a new C++ file at src/hello.cc 
with a Hello World program that includes iostream and prints "Hello, World!" to console.
Make sure to create the parent directories if they don't exist.
```

#### 提示词 C - JSON 格式（高级）
```
Execute this tool call:
Tool: agent_file_writer
Parameters:
{
  "operation": "write_single",
  "path": "src/hello.cc",
  "content": "#include <iostream>\n\nint main() {\n    std::cout << \"Hello, World!\" << std::endl;\n    return 0;\n}\n",
  "create_dirs": true,
  "backup": true
}
```

### 步骤 3: 执行并观察结果

Agent 应该：
1. 识别需要使用 `agent_file_writer` 工具
2. 调用工具并传递参数
3. 显示返回结果

---

## ✨ 预期输出示例

### 工具执行结果
```json
{
  "path": "src/hello.cc",
  "size": 87,
  "status": "success"
}
```

### Agent 响应
```
I've successfully created a C++ "Hello, World!" program at src/hello.cc.

The file contains:
- Standard iostream header for console output
- A main function that prints "Hello, World!" to the console
- Proper return statement

The file has been created in the src/ directory with appropriate C++ formatting.
You can compile and run it using:
  g++ src/hello.cc -o hello
  ./hello
```

---

## 🔍 验证文件是否成功创建

打开终端并执行：

```bash
# 检查文件是否存在
ls -lh /Users/feifei/agent/neurx-code/src/hello.cc

# 查看文件内容
cat /Users/feifei/agent/neurx-code/src/hello.cc

# 编译并运行（可选）
cd /Users/feifei/agent/neurx-code
g++ src/hello.cc -o hello
./hello
```

---

## 🛠️ 工具功能总结

| 功能 | 说明 |
|-----|------|
| **write_single** | 写入单个文件 |
| **write_batch** | 批量原子写入多个文件 |
| **update_file** | 追加/前置/插入到现有文件 |
| **write_template** | 从模板生成文件 |
| **create_structure** | 创建整个目录结构 |

---

## 💡 其他测试场景

### 场景 2: 批量写入（创建完整项目）

```
Create a complete C++ project with multiple files:
1. src/main.cpp - Main program
2. src/hello.h - Header file
3. include/config.h - Configuration header
4. CMakeLists.txt - Build file
```

### 场景 3: 更新现有文件

```
Append a comment to src/hello.cc explaining what the program does
```

### 场景 4: 创建项目结构

```
Create a C++ project directory structure with src/, include/, and build/ directories
```

---

## 📊 状态检查

### 工具是否已注册？

在 Agent 中输入：
```
List all available tools
```

输出应包含 `agent_file_writer`

### 工具参数是否正确？

在 Agent 中输入：
```
Show the schema for agent_file_writer tool
```

应该显示完整的参数定义

---

## ✅ 测试检查清单

- [ ] Agent 应用已启动
- [ ] 能看到 Agent 输入窗口
- [ ] 提示词已输入
- [ ] Agent 识别了 `agent_file_writer` 工具调用
- [ ] 工具执行成功（无错误）
- [ ] 返回结果包含 `"status": "success"`
- [ ] 文件 `/Users/feifei/agent/neurx-code/src/hello.cc` 已创建
- [ ] 文件内容正确（包含 C++ 代码）
- [ ] 可以编译文件（可选）
- [ ] 可以运行文件输出 "Hello, World!"（可选）

---

## 🐛 常见问题排查

### 问题 1: 工具未被调用

**症状**: Agent 没有使用 `agent_file_writer` 工具

**解决方案**:
- 确保在提示词中明确提及"write file" 或 "create file"
- 检查 Agent 日志是否显示工具注册
- 尝试直接指定工具名称

### 问题 2: 文件未被创建

**症状**: 工具执行成功但文件不存在

**解决方案**:
- 检查 `src/` 目录是否存在（通常不存在需要创建）
- 确认文件路径是否正确
- 检查工作区权限

### 问题 3: 内容验证失败

**症状**: "Validation failed" 错误

**解决方案**:
- 检查内容是否包含空字节 
- 确认内容大小不超过 10 MB
- 尝试简化内容

---

## 📞 调试信息

如需查看详细的调试信息，在终端中查看：

```bash
# 查看应用日志
tail -f /tmp/neurx-code-debug.log

# 检查工具注册
ps aux | grep neurx-codeApp
```

---

## 🎯 下一步

成功测试后，可以尝试：

1. ✅ 创建更复杂的文件结构
2. ✅ 使用批量操作创建多个文件
3. ✅ 从模板生成代码
4. ✅ 更新现有文件内容
5. ✅ 集成到自动化工作流中

---

**状态**: ✅ 集成完成，准备测试
