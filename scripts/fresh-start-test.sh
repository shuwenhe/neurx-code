#!/bin/bash

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔄 neurx-code 全新启动测试"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 1. 杀掉所有现有进程
echo "1️⃣  停止所有neurx-code进程..."
pkill -9 neurx-codeApp 2>/dev/null
sleep 2

# 2. 验证编译时间
echo ""
echo "2️⃣  验证二进制版本..."
BINARY="/Users/feifei/agent/neurx-code/build/Qt_6_10_3_for_macOS-Debug/neurx-codeApp.app/Contents/MacOS/neurx-codeApp"
BUILD_TIME=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$BINARY")
BUILD_SIZE=$(ls -lh "$BINARY" | awk '{print $5}')
echo "   编译时间: $BUILD_TIME"
echo "   文件大小: $BUILD_SIZE"

# 3. 验证关键修复
echo ""
echo "3️⃣  验证关键修复是否在二进制中..."
if strings "$BINARY" | grep -q "REAL FILE SYSTEM TOOL"; then
    echo "   ✅ 工具描述修复已包含"
else
    echo "   ❌ 工具描述修复未找到"
fi

if strings "$BINARY" | grep -q "CRITICAL.*All tools.*REAL and FUNCTIONAL"; then
    echo "   ✅ System Prompt修复已包含"
else
    echo "   ❌ System Prompt修复未找到"
fi

# 4. 清除chat历史（如果可能）
echo ""
echo "4️⃣  清除旧的chat历史..."
CHAT_HISTORY_DIR="$HOME/.config/neurx-code"
if [ -d "$CHAT_HISTORY_DIR" ]; then
    echo "   找到配置目录: $CHAT_HISTORY_DIR"
    echo "   备份并清除历史..."
    BACKUP_DIR="$CHAT_HISTORY_DIR.backup.$(date +%Y%m%d_%H%M%S)"
    mv "$CHAT_HISTORY_DIR" "$BACKUP_DIR" 2>/dev/null && echo "   ✅ 已备份到: $BACKUP_DIR" || echo "   ⚠️  无需清除"
else
    echo "   ℹ️  未找到配置目录，可能在其他位置"
fi

# 5. 启动neurx-code
echo ""
echo "5️⃣  启动neurx-code..."
cd /Users/feifei/agent/neurx-code
"$BINARY" > /tmp/neurx-code-output.log 2>&1 &
NEURX_PID=$!
echo "   ✅ neurx-code已启动 (PID: $NEURX_PID)"

# 6. 等待启动
echo ""
echo "6️⃣  等待应用完全启动..."
sleep 5

# 验证进程
if ps -p $NEURX_PID > /dev/null; then
    echo "   ✅ 进程运行正常"
else
    echo "   ❌ 进程启动失败，查看日志: tail /tmp/neurx-code-output.log"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📋 测试步骤"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "⚠️  CRITICAL: 必须使用全新的对话！"
echo ""
echo "1️⃣  在neurx-code中开始一个【全新的对话】"
echo "    → 不要使用旧的chat历史"
echo "    → 点击 'New Chat' 或类似按钮"
echo ""
echo "2️⃣  配置workspace（如果还没配置）"
echo "    → File → Open Workspace"
echo "    → 选择: /Users/feifei/agent/neurx-code"
echo ""
echo "3️⃣  启用Auto-approve（如果还没启用）"
echo "    → Settings → Safety → Auto-approve safe tools"
echo ""
echo "4️⃣  在【全新对话】中输入测试命令:"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  创建src/final_verification.cc文件，写入Hello World C++程序"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "5️⃣  观察agent的响应，应该:"
echo "    ✅ 调用Write或agent_file_writer工具"
echo "    ✅ 显示工具调用参数"
echo "    ✅ 报告文件创建成功"
echo ""
echo "    ❌ 不应该说 '无法在真实环境中执行'"
echo "    ❌ 不应该说 '仅在模拟环境'"
echo "    ❌ 不应该说 'cannot create files'"
echo ""
echo "6️⃣  验证文件是否真的创建了:"
echo ""
echo "    ls -lh /Users/feifei/agent/neurx-code/src/final_verification.cc"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "🔍 如果仍然失败，请截图agent的完整响应并告诉我:"
echo "   1. 使用的是哪个LLM provider (Ollama/OpenAI/Anthropic)"
echo "   2. 使用的是哪个模型"
echo "   3. Agent的完整回复内容"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
