#!/bin/bash

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║      neurx-code 日志捕获测试（实时诊断）                    ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# 停止旧进程
OLD_PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')
if [ -n "$OLD_PID" ]; then
    echo "🛑 停止旧进程 (PID: $OLD_PID)..."
    kill $OLD_PID
    sleep 2
fi

# 启动最新版本并捕获日志
BINARY=$(find /Users/feifei/agent/neurx-code/build -name neurx-codeApp -type f -perm +111 -print0 2>/dev/null \
    | xargs -0 ls -t 2>/dev/null \
    | head -1)
LOG_FILE="/tmp/neurx-full.log"

if [ -z "$BINARY" ] || [ ! -f "$BINARY" ]; then
    echo "❌ 找不到 neurx-codeApp 二进制"
    exit 1
fi

echo "✅ 启动neurx-code并记录日志到: $LOG_FILE"
echo "📦 使用二进制: $BINARY"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📋 测试步骤："
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1️⃣  等待neurx-code窗口出现（约5秒）"
echo ""
echo "2️⃣  在neurx-code UI中配置："
echo "    ✓ File → Open Workspace → /Users/feifei/agent/neurx-code"
echo "    ✓ Settings → Safety → 打开 'Auto-approve safe tools'"
echo ""
echo "3️⃣  在聊天框输入测试命令："
echo "    创建src/demo.cc，内容是Hello World C++程序"
echo ""
echo "4️⃣  在另一个终端窗口监控日志（新开一个终端）："
echo "    tail -f $LOG_FILE | grep --color=always -E 'AgentFileWriterTool|agent_file_writer|toolCall|execute.*operation'"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "按回车启动neurx-code..."
read -r

echo "🚀 启动中..."
"$BINARY" 2>&1 | tee "$LOG_FILE" &
APP_PID=$!

sleep 3

echo ""
echo "✅ neurx-code已启动 (PID: $APP_PID)"
echo "📝 日志文件: $LOG_FILE"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔍 实时监控工具调用（在新终端运行）："
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  tail -f $LOG_FILE | grep --color=always -E 'AgentFileWriterTool|agent_file_writer|toolCall|execute.*operation|write.*file'"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "💡 发送测试命令后，查看日志分析："
echo ""
echo "• 如果看到 'AgentFileWriterTool execute' → 工具被调用了"
echo "• 如果看到 'operation: write_single' → 参数正确"
echo "• 如果看到 'File written successfully' → 写入成功"
echo "• 如果看到 'System prompt refreshed ... realToolGuardrails=true' → 新 prompt 已生效"
echo "• 如果什么都没看到 → LLM没有调用工具（workspace未设置）"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "测试完成后，按 Ctrl+C 停止日志记录"
echo "然后运行: cat $LOG_FILE | grep -A10 -B10 agent_file_writer"
echo ""
