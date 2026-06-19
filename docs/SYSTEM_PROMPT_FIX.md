# neurx-code Agent 文件创建问题 - 最终解决方案

## 问题根源

通过用户提供的截图确认，neurx-code的agent回复说：
> "未实际写入（需用户手动执行上述命令）"  
> "当前环境无法直接操作用户的本地文件系统"  
> "Write工具仅在模拟环境中可用"

**诊断结果**：这是**LLM的误解**，不是代码问题。

---

## 根本原因分析

### 为什么LLM会误判？

1. **System Prompt不够明确**
   - 原system prompt只是列出了工具，但没有强调这些工具是**真实可用**的
   - LLM基于训练数据中的模式，误判为"模拟环境"的工具

2. **LLM训练数据的影响**
   - 很多训练数据中的assistant被限制为"只能提供代码，不能直接操作文件"
   - LLM学习到了这种模式并错误应用到neurx-code中

3. **工具列表中的"Claude Standard"标签**
   - System prompt中写了"Claude Standard File Operations"
   - LLM可能误以为这些是Claude的sandbox工具，而不是真实的文件系统操作

---

## 解决方案

### 修改的文件

**1. `/Users/feifei/agent/neurx-code/src/bridge/AgentController.cpp`**

```cpp
// 在system prompt开头添加明确的指令：
static const QString kControllerSystemPrompt = R"(
You are NeurX Code, an expert software engineering AI assistant.
You are operating as a code agent, not a chat assistant.
You have access to tools that let you read and write files, apply patches,
run shell commands (both local and sandboxed Docker), and search the codebase.

CRITICAL: All tools listed below are REAL and FUNCTIONAL. When the user asks you to create,
edit, or write files, you MUST use the appropriate tool (Write, agent_file_writer, Edit, etc.)
to actually perform the operation. DO NOT just show code - EXECUTE the tool to create/modify files.
These tools directly interact with the user's local filesystem and will create real files.
...
```

```cpp
// 在Guidelines部分强调：
Guidelines:
- IMPORTANT: When user asks to create/write a file, you MUST actually use Write or agent_file_writer tool to create it. Do not just show code without calling the tool.
- For simple file creation, use 'Write' tool or 'agent_file_writer' tool - both create REAL files on disk.
...
```

**2. `/Users/feifei/agent/neurx-code/src/agent/AgentEngine.cpp`**

```cpp
// 同样添加明确的指令：
static const QString kDefaultSystem = R"(
You are NeurX Code, an expert software engineering AI assistant.
You have access to tools that let you read and write files, run shell commands,
and search the codebase. Use them to complete coding tasks accurately.

CRITICAL: All tools you have access to are REAL and FUNCTIONAL. When the user asks you
to create, edit, or write files, you MUST use the appropriate tool to actually perform
the operation. DO NOT just show code - EXECUTE the tool to create/modify files.
Your tools directly interact with the user's local filesystem and will create real files.

Guidelines:
- IMPORTANT: When user asks to create/write a file, you MUST actually use the file writing tool to create it.
...
```

---

## 修复内容总结

### 添加的关键指令

1. **CRITICAL声明**
   - 明确告诉LLM：所有工具都是REAL and FUNCTIONAL
   - 强调工具直接与用户本地文件系统交互

2. **明确的行为指令**
   - "DO NOT just show code - EXECUTE the tool"
   - "you MUST use the appropriate tool to actually perform the operation"

3. **Guidelines中的重复强调**
   - 在Guidelines第一条就强调必须实际调用工具
   - 明确说明Write和agent_file_writer都能创建真实文件

---

## 测试方法

运行测试脚本：
```bash
chmod +x /Users/feifei/agent/neurx-code/test-system-prompt-fix.sh
/Users/feifei/agent/neurx-code/test-system-prompt-fix.sh
```

测试步骤：
1. 脚本会重启neurx-code
2. 在neurx-code聊天框输入：`创建src/test_fix.cc文件，内容是简单的Hello World C++程序`
3. 观察agent的响应：
   - ✅ 应该调用Write或agent_file_writer工具
   - ✅ 不应该说"无法操作文件系统"
   - ✅ 文件应该被实际创建

验证：
```bash
ls -lh /Users/feifei/agent/neurx-code/src/test_fix.cc
cat /Users/feifei/agent/neurx-code/src/test_fix.cc
```

---

## 为什么这个修复有效？

### LLM行为改变机制

1. **System Prompt的权威性**
   - LLM高度重视system prompt中的明确指令
   - "CRITICAL"和"MUST"等强调词增加指令权重

2. **消除歧义**
   - 明确说明工具是"REAL"而不是"模拟"
   - 直接指出要"EXECUTE the tool"而不是"just show code"

3. **重复强化**
   - 在system prompt开头和Guidelines中都强调
   - 多次重复同一概念增加LLM遵循的可能性

---

## 总结

### 问题本质
- ❌ 不是代码问题（所有工具代码都正常）
- ❌ 不是编译问题（编译一直是成功的）
- ✅ 是LLM误解了自己的能力范围

### 解决方案
- ✅ 通过明确的System Prompt指令纠正LLM的误解
- ✅ 强调工具的真实性和必须使用性
- ✅ 消除训练数据带来的限制性偏见

### 关键经验
1. **代码正确 ≠ 功能可用**：LLM-based系统中，prompt设计和代码实现同样重要
2. **明确性至关重要**：不能假设LLM理解隐含的意图，必须明确说明
3. **对抗训练偏见**：需要用更强的prompt指令覆盖训练数据中的限制性模式

---

## 编译和部署

编译时间：2026-06-18（最新）
二进制位置：`/Users/feifei/agent/neurx-code/build/Qt_6_10_3_for_macOS-Debug/neurx-codeApp.app/Contents/MacOS/neurx-codeApp`

重启neurx-code后，新的system prompt立即生效。

---

## 后续建议

如果仍然遇到问题，可能需要：
1. 检查UI配置（workspace路径和auto-approve）
2. 验证使用的LLM模型是否支持工具调用
3. 查看agent日志确认工具是否被调用
