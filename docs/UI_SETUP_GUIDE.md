# 📱 neurx-code UI 设置指南

## ❌ 当前问题
Agent生成了代码但文件没有创建，原因是缺少关键设置。

---

## ✅ 解决方案：设置工作空间路径

### 方法1: 在UI中设置工作空间路径（推荐）

neurx-code的工作空间路径设置可能在以下位置之一：

#### 选项A: 文件树面板  
1. 查看窗口左侧的**文件浏览器**面板
2. 如果显示"No workspace"或空白，说明未设置工作空间
3. 可能有一个"打开文件夹"或"选择工作空间"按钮

#### 选项B: 标题栏显示
窗口标题会显示：`NeurX Code — <工作空间路径>` 或 `No workspace`
- 如果显示"No workspace"，需要设置

#### 选项C: 通过QML控制台设置
如果UI中找不到设置入口，可以通过QML控制台直接设置：

1. 打开终端，找到运行的neurx-codeApp进程
2. 或者重启neurx-code并在启动时设置

---

## 🔑 关键设置清单

### 1. 工作空间路径 ⭐ 必须设置
```
建议设置为: /tmp/neurx-test-workspace
```

**如何检查:**
- 查看窗口标题：应显示路径而不是"No workspace"
- 查看文件树面板：应显示文件和目录

### 2. Auto-approve 工具 ⭐ 必须启用
```
位置: Settings → Safety → "Auto-approve safe tools"
```

**如何设置:**
1. 点击右上角的"Settings"按钮（⚙️图标）
2. 滚动到"Safety"部分
3. 找到"Auto-approve safe tools"开关
4. 确保开关是**打开**状态

**说明文字:**（UI中显示的内容）
"When enabled, read-only tools can run automatically. Commands, patches, and file writes still ask first."

**⚠️ 注意:** 这个说明文字已过时！根据我们的修复，agent_file_writer现在是medium风险，启用此选项后会自动执行，无需批准。

---

## 🧪 测试步骤

### 步骤1: 创建测试工作空间
```bash
mkdir -p /tmp/neurx-test-workspace
```

### 步骤2: 在neurx-code中设置
在neurx-code UI中将工作空间设置为 `/tmp/neurx-test-workspace`

### 步骤3: 启用Auto-approve
Settings → Safety → 打开"Auto-approve safe tools"

### 步骤4: 测试文件创建
在聊天框输入：
```
创建hello.cc文件，写入一个简单的Hello World C++程序
```

### 步骤5: 验证结果
在终端运行：
```bash
ls -lh /tmp/neurx-test-workspace/hello.cc
cat /tmp/neurx-test-workspace/hello.cc
```

---

## 🔍 如果UI中找不到工作空间设置

### 临时解决方案：使用QML命令

如果UI中没有明显的"选择工作空间"按钮，可以尝试以下方法：

#### 方法1: 检查快捷键
- 查看是否有"Open Folder"或"Select Workspace"快捷键
- 常见快捷键: Ctrl+K Ctrl+O, Ctrl+O

#### 方法2: 检查菜单
- 查找"File"菜单或类似的菜单项
- 可能有"Open Workspace"选项

#### 方法3: 从Agent直接设置（如果支持）
在聊天框尝试：
```
设置工作空间路径为 /tmp/neurx-test-workspace
```

#### 方法4: 拖拽文件夹
某些应用支持将文件夹拖拽到应用窗口来设置工作空间

---

## 📋 期望的Agent响应

### 成功的响应应该包含:

1. **LLM生成代码**
   ```
   我来创建hello.cc文件：
   #include <iostream>
   int main() {
       std::cout << "Hello, World!" << std::endl;
       return 0;
   }
   ```

2. **工具调用信息**（可能显示）
   ```
   Using tool: agent_file_writer
   ```

3. **成功消息**
   ```
   ✅ 文件已创建: hello.cc
   ```

### ❌ 问题症状

**症状A: 没有任何响应**
→ 检查Provider和API key配置
→ 查看是否有错误消息

**症状B: 只有代码文本，没有文件**
→ 工作空间路径未设置
→ Auto-approve未启用
→ LLM没有生成工具调用

**症状C: 显示批准对话框**
→ Auto-approve未启用，或
→ 使用的是旧版本（需重新编译）

---

## 🎯 完整设置检查列表

- [ ] neurx-codeApp版本: 2026-06-18 11:34或更新
- [ ] 工作空间路径已设置（窗口标题显示路径）
- [ ] Settings → Safety → "Auto-approve safe tools" = ON
- [ ] Provider已选择（OpenAI/Anthropic/Gemini）
- [ ] API Key已配置
- [ ] Model已选择（支持工具调用的模型）
- [ ] 工作空间目录存在且有写权限

---

## 💡 快速诊断命令

```bash
# 1. 检查neurx-code版本
ls -lh /Users/feifei/agent/neurx-code/build/Qt_6_10_3_for_macOS-Debug/neurx-codeApp.app/Contents/MacOS/neurx-codeApp

# 应显示: Jun 18 11:34 或更新

# 2. 创建测试工作空间
mkdir -p /tmp/neurx-test-workspace

# 3. 监控文件创建
watch -n 1 "ls -lh /tmp/neurx-test-workspace/"

# 4. 检查最近创建的文件
find /tmp -name "hello.cc" -mmin -5 2>/dev/null
```

---

## 🆘 仍然无法工作？

如果按照以上步骤仍然无法工作，请提供以下信息：

1. **窗口标题显示的内容**（特别是工作空间部分）
2. **Settings中Auto-approve的状态**（开/关）
3. **Agent的实际响应**（完整文本）
4. **是否有任何错误消息**
5. **当前使用的Provider和Model**

---

**编译版本**: 2026-06-18 11:34  
**修复状态**: ✅ 所有代码修复已应用  
**待解决**: UI设置配置
