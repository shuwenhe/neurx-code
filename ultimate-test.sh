#!/bin/bash

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║     neurx-code Agent 终极修复测试                            ║"
echo "║     包含: System Prompt + 工具描述双重修复                   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

# 杀掉所有旧进程
echo "🛑 停止所有旧的neurx-code进程..."
killall neurx-codeApp 2>/dev/null
sleep 3

# 检查新编译的二进制
BINARY="/Users/feifei/agent/neurx-code/build/Qt_6_10_3_for_macOS-Debug/neurx-codeApp.app/Contents/MacOS/neurx-codeApp"
if [ ! -f "$BINARY" ]; then
    echo "❌ 找不到编译的二进制文件: $BINARY"
    exit 1
fi

COMPILE_TIME=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" "$BINARY" 2>/dev/null)
FILE_SIZE=$(ls -lh "$BINARY" | awk '{print $5}')

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ 新版本信息:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "   📍 位置: $BINARY"
echo "   ⏰ 编译: $COMPILE_TIME"
echo "   📦 大小: $FILE_SIZE"
echo ""

# 检查修复内容
echo "🔍 验证修复内容..."
if strings "$BINARY" | grep -q "REAL FILE SYSTEM TOOL"; then
    echo "   ✅ 工具描述修复已包含"
else
    echo "   ⚠️  工具描述修复未检测到（可能是strings限制）"
fi

if strings "$BINARY" | grep -q "CRITICAL.*All tools"; then
    echo "   ✅ System Prompt修复已包含"
else
    echo "   ⚠️  System Prompt修复未检测到（可能是strings限制）"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🚀 启动最新版本..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

"$BINARY" &
APP_PID=$!

sleep 4

if ps -p $APP_PID > /dev/null 2>&1; then
    echo "✅ neurx-code已启动 (PID: $APP_PID)"
else
    echo "❌ neurx-code启动失败"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📋 测试步骤"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1️⃣  配置neurx-code（必须完成）"
echo ""
echo "    a) 设置工作空间:"
echo "       → File → Open Workspace"
echo "       → 选择: /Users/feifei/agent/neurx-code"
echo "       → 确认窗口标题显示路径"
echo ""
echo "    b) 启用Auto-approve:"
echo "       → 点击右上角 ⚙️ Settings"
echo "       → 找到 Safety 部分"
echo "       → 打开 'Auto-approve safe tools' 开关"
echo "       → 关闭设置"
echo ""
echo "2️⃣  在neurx-code聊天框输入以下命令:"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  创建src/final_test.cc文件，内容是Hello World C++程序"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "3️⃣  观察agent的响应，应该包含:"
echo ""
echo "    ✅ \"使用Write工具\" 或 \"调用agent_file_writer\""
echo "    ✅ 显示工具调用的参数"
echo "    ✅ \"文件已创建\" 或类似成功消息"
echo ""
echo "    ❌ 不应该说 \"无法操作文件系统\""
echo "    ❌ 不应该说 \"仅在模拟环境\""
echo "    ❌ 不应该说 \"需手动执行\""
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "⏱️  准备好后按回车，开始监控文件创建（40秒）..."
read -r

TARGET="/Users/feifei/agent/neurx-code/src/final_test.cc"
[ -f "$TARGET" ] && rm -f "$TARGET"

echo ""
echo "🔍 监控中: $TARGET"
echo ""

COUNT=0
while [ $COUNT -lt 40 ]; do
    if [ -f "$TARGET" ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🎉🎉🎉 成功！文件已创建！"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "📁 文件信息:"
        ls -lh "$TARGET"
        echo ""
        echo "📄 文件内容:"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        cat "$TARGET"
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "✅✅✅ neurx-code Agent 文件创建功能已完全修复！"
        echo ""
        echo "🔧 应用的修复:"
        echo "   1. System Prompt添加CRITICAL指令"
        echo "   2. 工具描述添加'REAL FILE SYSTEM TOOL'说明"
        echo "   3. 多处强调工具是真实可用的"
        echo ""
        echo "🧪 测试编译:"
        echo "   cd /Users/feifei/agent/neurx-code/src"
        echo "   g++ -std=c++17 final_test.cc -o final_test && ./final_test"
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
echo "⚠️  40秒内未检测到文件创建"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "🔍 请告诉我agent的具体响应是什么："
echo ""
echo "选项A: Agent说 \"无法操作文件系统\" 或 \"仅在模拟环境\""
echo "   → 说明LLM仍然误解，可能需要更换模型或检查是否用了旧版本"
echo ""
echo "选项B: Agent说要使用工具，但文件没创建"
echo "   → 工具被调用但失败，可能是workspace未设置或auto-approve未启用"
echo ""
echo "选项C: Agent只显示代码，没提工具"
echo "   → Workspace路径未设置"
echo ""
echo "选项D: Agent完全没响应"
echo "   → Provider配置问题"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "💡 快速验证："
echo ""
echo "1. 确认neurx-code窗口标题显示: /Users/feifei/agent/neurx-code"
echo ""
echo "2. 在聊天框输入简单命令测试Provider："
echo "   \"你好，你是什么？\""
echo "   （如果没响应 → Provider问题）"
echo ""
echo "3. 查看agent的完整响应，复制给我分析"
echo ""
