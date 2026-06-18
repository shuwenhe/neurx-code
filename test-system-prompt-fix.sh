#!/bin/bash

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║      neurx-code System Prompt 修复测试                       ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# 停止旧进程
echo "🛑 停止旧的neurx-code进程..."
OLD_PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')
if [ -n "$OLD_PID" ]; then
    kill $OLD_PID
    sleep 2
    echo "   ✅ 已停止 PID: $OLD_PID"
else
    echo "   ℹ️  没有运行中的进程"
fi

# 查找最新编译的二进制，避免启动旧版本
BINARY=$(find /Users/feifei/agent/neurx-code/build -name neurx-codeApp -type f -perm +111 -print0 2>/dev/null \
    | xargs -0 ls -t 2>/dev/null \
    | head -1)
if [ -z "$BINARY" ] || [ ! -f "$BINARY" ]; then
    echo "❌ 找不到编译的二进制文件"
    exit 1
fi

COMPILE_TIME=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$BINARY" 2>/dev/null)
FILE_SIZE=$(ls -lh "$BINARY" | awk '{print $5}')

echo ""
echo "✅ 新版本信息:"
echo "   二进制路径: $BINARY"
echo "   编译时间: $COMPILE_TIME"
echo "   文件大小: $FILE_SIZE"
echo ""

# 检查修复内容
if strings "$BINARY" | grep -q "Never claim that file-writing tools are only theoretical"; then
    echo "✅ System Prompt修复已包含在二进制中"
else
    echo "⚠️  未检测到新的 prompt 标记，可能仍在运行旧版本"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🚀 启动新版本neurx-code"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

"$BINARY" &
APP_PID=$!

sleep 3

if ps -p $APP_PID > /dev/null 2>&1; then
    echo "✅ neurx-code已启动 (PID: $APP_PID)"
else
    echo "❌ neurx-code启动失败"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📋 现在请完成以下测试步骤："
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1️⃣  配置neurx-code（如果还没配置）"
echo "    • File → Open Workspace → /Users/feifei/agent/neurx-code"
echo "    • Settings → Safety → 打开 'Auto-approve safe tools'"
echo ""
echo "2️⃣  测试文件创建（在neurx-code聊天框输入）"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  创建src/test_fix.cc文件，内容是简单的Hello World C++程序"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "3️⃣  预期结果"
echo "    • Agent应该**调用Write或agent_file_writer工具**"
echo "    • 不应该说'无法直接操作文件系统'或'仅在模拟环境中可用'"
echo "    • 应该实际创建文件"
echo "    • 日志里应出现: [AgentController] System prompt refreshed ... realToolGuardrails=true"
echo ""
echo "4️⃣  验证文件创建（10秒后自动检查）"

TARGET="/Users/feifei/agent/neurx-code/src/test_fix.cc"
[ -f "$TARGET" ] && rm -f "$TARGET"

echo ""
echo "等待文件创建... (监控30秒)"
echo "目标: $TARGET"
echo ""

COUNT=0
while [ $COUNT -lt 30 ]; do
    if [ -f "$TARGET" ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🎉🎉🎉 成功！System Prompt修复生效！"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "📁 文件信息:"
        ls -lh "$TARGET"
        echo ""
        echo "📄 文件内容:"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        cat "$TARGET"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "✅ neurx-code Agent现在可以正常创建文件了！"
        echo ""
        echo "问题已解决："
        echo "• System Prompt中添加了明确的指令"
        echo "• LLM现在知道工具是真实可用的"
        echo "• Agent会调用工具而不是只显示代码"
        echo ""
        exit 0
    fi
    
    echo -n "."
    sleep 1
    COUNT=$((COUNT + 1))
    
    if [ $((COUNT % 10)) -eq 0 ]; then
        echo " ${COUNT}s"
    fi
done

echo ""
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "⚠️  30秒内未检测到文件"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "可能的原因："
echo ""
echo "❓ 还没在neurx-code中发送命令？"
echo "   → 在聊天框输入: 创建src/test_fix.cc文件，内容是简单的Hello World C++程序"
echo ""
echo "❓ Workspace路径未设置？"
echo "   → File → Open Workspace → /Users/feifei/agent/neurx-code"
echo ""
echo "❓ Auto-approve未启用？"
echo "   → Settings → Safety → 打开 'Auto-approve safe tools'"
echo ""
echo "❓ Agent的响应是什么？"
echo "   • 如果说'无法操作文件系统' → System prompt可能未生效，重启neurx-code"
echo "   • 如果说要使用工具 → 很好，只是需要配置workspace或auto-approve"
echo "   • 如果只显示代码 → 检查窗口标题是否显示workspace路径"
echo ""
