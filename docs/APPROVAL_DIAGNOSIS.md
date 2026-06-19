# ⚠️ neurx-code Agent 文件写入问题诊断

## 问题
OpenAI Provider (SiliconFlow + Qwen3-32B) 生成的代码没有写入文件

## 根本原因

**工具调用需要批准，但批准对话框可能被忽略或未显示**

### 关键发现

1. ✅ **OpenAI Provider支持工具调用** - 已验证
2. ✅ **agent_file_writer工具已注册** - 已验证  
3. ⚠️  **agent_file_writer被分类为"high"风险** - 需要批准
4. ⚠️  **默认设置: autoApproveTools = false** - 所有工具都需要批准
5. ⚠️  **即使启用autoApproveTools，"high"风险工具仍需批准**

### 代码证据

```cpp
// src/bridge/AgentController.cpp:2618
if (name == QStringLiteral("agent_file_writer")
    || name == QStringLiteral("file_system")
    || name == QStringLiteral("codex_file_system")
    || ...)
    return QStringLiteral("high");  // ← 高风险！
```

```cpp
// src/bridge/AgentController.h:619
bool m_autoApproveTools{false};  // ← 默认需要批准
```

```cpp
// src/bridge/AgentController.cpp:2684
return !m_autoApproveTools 
    || risk == QStringLiteral("high") 
    || risk == QStringLiteral("critical");  // ← high风险总是需要批准
```

---

## 解决方案

### 方案1: 启用自动批准（推荐快速测试）

**步骤：**
1. 在neurx-code中，打开 **Settings**（设置）
2. 找到 **Safety** 部分
3. 启用 **"Auto-approve safe tools"** 开关
4. 注意：这只会自动批准低风险工具，文件写入仍需手动批准

### 方案2: 手动批准工具调用（正确流程）

**步骤：**
1. 发送命令：`创建hello.cc文件...`
2. Agent会生成工具调用
3. **弹出批准对话框** - 这是关键！
4. 在对话框中点击 **"Approve"** 按钮
5. 文件将被写入

**如果没有看到对话框：**
- 对话框可能在窗口后面
- 检查窗口焦点
- 查看日志输出

### 方案3: 修改代码（降低风险级别）

如果你想让`agent_file_writer`自动批准，可以修改代码：

**编辑 src/bridge/AgentController.cpp:2618-2629**

```cpp
// 选项A: 将agent_file_writer改为medium风险
if (name == QStringLiteral("agent_file_writer"))
    return QStringLiteral("medium");  // 改为medium，启用auto-approve后会自动批准

// 原来的代码：
if (name == QStringLiteral("Write")
    || name == QStringLiteral("file_system")
    || name == QStringLiteral("codex_file_system")
    // || name == QStringLiteral("agent_file_writer")  // 注释掉这行
    || name == QStringLiteral("file_creation")
    || ...)
    return QStringLiteral("high");
```

**重新编译：**
```bash
cd /Users/feifei/agent/neurx-code
cmake --build build --target neurx-codeApp -j4
```

---

## 诊断步骤

### 检查1: 查看日志
启动neurx-code并查看控制台输出：

**期望看到：**
```
[AgentToolRegistry] Registering tool: "agent_file_writer"
[Planner] Built X tools for provider: openai
[agent] response received: toolCalls=1
[AgentController] Tool approval required: agent_file_writer (high risk)
```

**如果看到"Tool approval required"但没有对话框：**
- 对话框实现有问题
- 或者被其他窗口遮挡

### 检查2: 验证工具调用生成
在Chat界面中，Agent的响应应该包含类似：

```json
Tool Call: agent_file_writer
{
  "operation": "write_single",
  "path": "hello.cc",
  "content": "..."
}
```

如果只看到文本描述而没有Tool Call，说明LLM没有生成工具调用（模型问题）。

### 检查3: 测试低风险工具
尝试一个低风险工具来验证批准机制：

```
列出工作空间的文件
```

这应该调用`list_dir`（低风险），如果启用了auto-approve应该自动执行。

---

## 快速修复（临时）

如果你想快速测试文件写入，可以临时禁用批准检查：

**编辑 src/bridge/AgentController.cpp:2684**

```cpp
// 临时修复：总是返回false（不需要批准）
return false;  // ← 添加这行，临时禁用所有批准

// 原来的代码（注释掉）：
// return !m_autoApproveTools 
//     || risk == QStringLiteral("high") 
//     || risk == QStringLiteral("critical");
```

**警告：** 这会禁用所有安全检查！只用于测试！

---

## 推荐流程

### 用户视角（正确使用方式）

1. **首次使用时：**
   - Settings → 启用 "Auto-approve safe tools"
   - 这允许读取文件、搜索等安全操作自动执行

2. **执行文件写入时：**
   - Agent会显示批准对话框
   - 查看要执行的操作
   - 点击 "Approve" 确认

3. **如果信任工作空间：**
   - 在批准对话框中可以选择 "Always approve for this workspace"
   - 后续操作将自动批准

### 开发者视角（调试）

1. **启动neurx-code时查看日志：**
   ```bash
   /Users/feifei/agent/neurx-code/build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp 2>&1 | tee neurx-debug.log
   ```

2. **发送测试命令：**
   ```
   创建hello.cc文件，内容为：
   #include <iostream>
   int main() {
       std::cout << "Hello, World!" << std::endl;
       return 0;
   }
   ```

3. **观察日志输出：**
   - 寻找 "toolApprovalRequired" 信号
   - 寻找 "ToolApprovalDialog::show" 调用
   - 寻找 "approveTool" 或 "rejectTool" 调用

4. **检查对话框：**
   - 是否显示了批准对话框？
   - 对话框的位置和可见性？

---

## 总结

**问题不是工具调用支持，而是安全批准机制！**

- ✅ OpenAI Provider完全支持工具调用
- ✅ agent_file_writer工具已正确注册
- ⚠️  工具需要用户批准才能执行
- ⚠️  用户可能没有看到或没有点击批准对话框

**下一步：**
1. 在neurx-code中发送文件创建命令
2. 等待并查找批准对话框
3. 点击"Approve"按钮
4. 验证文件是否被创建

如果对话框确实没有显示，那可能是UI问题，需要进一步调查。
