# 快速测试指南

## 🚀 立即测试

### 步骤1: 启动neurx-code
```bash
/Users/feifei/agent/neurx-code/build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp
```

### 步骤2: 配置
1. **设置工作空间**: 选择一个空目录（例如 `/tmp/test-workspace`）
2. **启用自动批准**: Settings → Safety → "Auto-approve safe tools" = **ON**
3. **选择Provider**: OpenAI (SiliconFlow) 或 Anthropic

### 步骤3: 测试命令
```
创建hello.cc文件，内容为Hello World程序
```

### 步骤4: 验证结果

#### 应该看到:
✅ Agent生成代码  
✅ 调用agent_file_writer工具  
✅ 文件自动创建（无需手动批准）  
✅ 显示成功消息  

#### 文件验证:
```bash
# 在工作空间目录中
ls -lh hello.cc
cat hello.cc
```

应该显示：
```cpp
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
```

---

## 🔍 查看详细日志

### 启动并捕获日志
```bash
/Users/feifei/agent/neurx-code/build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp 2>&1 | tee /tmp/neurx-debug.log
```

### 在另一个终端查看日志
```bash
tail -f /tmp/neurx-debug.log | grep -E "AgentFileWriterTool|tool result"
```

### 期望看到的日志
```
[AgentFileWriterTool] No operation specified, defaulting to write_single
[AgentFileWriterTool] execute() called with operation: write_single path: hello.cc
[AgentFileWriterTool::opWriteSingle] Writing to: hello.cc size: 123 bytes
[AgentFileWriterTool::opWriteSingle] File written successfully: /tmp/test-workspace/hello.cc size: 123 bytes
[agent] tool result: agent_file_writer callId=call_xxx error=false
```

---

## ❌ 故障排除

### 问题: 文件没有创建

#### 检查1: 工具是否被调用？
```bash
grep "AgentFileWriterTool" /tmp/neurx-debug.log
```

**如果没有输出:**
- LLM没有生成工具调用
- 尝试更明确的提示: "使用agent_file_writer工具创建hello.cc文件"
- 切换到支持工具调用的模型

#### 检查2: Auto-approve是否启用？
Settings → Safety → "Auto-approve safe tools" 必须为 **ON**

#### 检查3: 工作空间路径是否设置？
必须先设置工作空间路径（在UI中）

#### 检查4: 查看错误日志
```bash
grep -E "error|Error|failed|Failed" /tmp/neurx-debug.log
```

### 问题: 需要手动批准

**可能原因:**
- Auto-approve未启用
- 使用的是旧版本（编译时间早于 2026-06-18 11:27）

**解决方法:**
```bash
# 检查可执行文件时间
ls -lh /Users/feifei/agent/neurx-code/build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp

# 如果时间早于 11:27，重新编译
cd /Users/feifei/agent/neurx-code
cmake --build build --target neurx-codeApp -j4
```

### 问题: 工具被调用但写入失败

**查看详细错误:**
```bash
grep "AgentFileWriterTool" /tmp/neurx-debug.log | grep -E "failed|error"
```

**常见原因:**
1. 路径穿越检测 - 使用相对路径
2. 权限问题 - 检查工作空间目录权限
3. 磁盘空间不足

---

## 📋 完整测试案例

### 测试1: 单文件创建
```
创建hello.cc文件，内容为Hello World程序
```

### 测试2: 多文件创建
```
创建main.cpp和util.h两个文件，一个是主程序，一个是工具头文件
```

### 测试3: 更新文件
```
将hello.cc中的World改为neurx-code
```

### 测试4: 创建目录结构
```
创建src/目录，并在其中创建main.cpp
```

---

## ✅ 成功标志

如果看到以下现象，说明修复成功：

1. ✅ 文件自动创建，**无需手动批准**
2. ✅ 日志显示 "defaulting to write_single"
3. ✅ 日志显示 "File written successfully"
4. ✅ 文件内容与Agent生成的代码一致
5. ✅ 可以连续创建多个文件

---

## 📊 性能指标

正常情况下：
- 响应时间: 2-5秒
- 文件创建: 即时（<100ms）
- CPU占用: 低
- 内存占用: ~400MB

---

## 🎯 下一步

修复验证后，可以测试更复杂的场景：
- 创建完整的C++项目结构
- 实现多文件程序
- 重构现有代码
- 添加新功能到现有文件

所有这些操作现在都应该**自动执行**，无需手动批准每一步！

---

**编译版本**: 2026-06-18 11:27  
**文件大小**: 55MB  
**修复状态**: ✅ 完整  
**测试准备**: ✅ 就绪
