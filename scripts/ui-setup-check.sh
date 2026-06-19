#!/bin/bash

cat << 'EOF'
╔══════════════════════════════════════════════════════════╗
║      neurx-code UI 设置指南（简化版）                    ║
╚══════════════════════════════════════════════════════════╝

当前状态
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
EOF

# 检查进程和版本
PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')
if [ -n "$PID" ]; then
    BINARY=$(ps -p $PID -o command= | awk '{print $1}')
    MOD_TIME=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$BINARY" 2>/dev/null)
    echo "✅ neurx-codeApp运行中 (PID: $PID)"
    echo "   编译时间: $MOD_TIME"
    
    if strings "$BINARY" | grep -q "No operation specified, defaulting"; then
        echo "✅ 包含所有代码修复"
    else
        echo "❌ 不包含修复"
        exit 1
    fi
else
    echo "❌ neurx-codeApp未运行，请先启动"
    exit 1
fi

# 创建测试工作空间
TEST_WS="/tmp/neurx-test-$(date +%s)"
mkdir -p "$TEST_WS"

cat << EOF

必做设置（请在neurx-code UI中操作）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

步骤1: 设置工作空间路径 ⭐ 最重要
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
方法: 查找UI中的"打开文件夹"或"选择工作空间"功能
路径: $TEST_WS

检查方法:
  - 窗口标题应显示工作空间路径
  - 如果显示"No workspace"，说明未设置

步骤2: 启用Auto-approve ⭐ 必须启用
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
位置: Settings → Safety → "Auto-approve safe tools"
状态: 必须打开（ON）

如何操作:
  1. 点击右上角的 Settings 按钮（⚙️图标）
  2. 滚动到"Safety"部分
  3. 找到"Auto-approve safe tools"开关
  4. 确保开关是打开状态

步骤3: 测试文件创建
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
在聊天框输入:

    创建hello.cc文件，写入Hello World C++程序

步骤4: 验证结果
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
EOF

echo "监控文件创建（30秒）..."
echo "工作空间: $TEST_WS"
echo ""
echo "如果文件创建成功，将自动显示内容"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 监控30秒
COUNT=0
while [ $COUNT -lt 30 ]; do
    if [ -f "$TEST_WS/hello.cc" ]; then
        echo ""
        echo "🎉 🎉 🎉  成功！文件已创建！ 🎉 🎉 🎉"
        echo ""
        ls -lh "$TEST_WS/hello.cc"
        echo ""
        echo "━━━━ 文件内容 ━━━━"
        cat "$TEST_WS/hello.cc"
        echo ""
        echo "━━━━━━━━━━━━━━━━━"
        echo ""
        echo "✅ 所有设置正确！Agent文件写入功能正常工作！"
        echo ""
        echo "下次使用时："
        echo "  - 工作空间路径: 设置为你的项目目录"
        echo "  - Auto-approve: 保持启用状态"
        exit 0
    fi
    echo -n "."
    sleep 1
    COUNT=$((COUNT + 1))
done

echo ""
echo ""
echo "❌ 30秒内未检测到文件创建"
echo ""
echo "问题排查:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1. 工作空间路径是否设置？"
echo "   → 查看窗口标题，应显示: $TEST_WS"
echo "   → 而不是'No workspace'"
echo ""
echo "2. Auto-approve是否启用？"
echo "   → 打开Settings → Safety"
echo "   → 确认'Auto-approve safe tools'开关是ON"
echo ""
echo "3. Agent有没有响应？"
echo "   → 如果完全没有响应，检查Provider和API key"
echo "   → 如果只返回文本没有创建文件，回到步骤1和2"
echo ""
echo "4. 检查可能在其他位置创建的文件:"
find /Users/feifei/agent/neurx-code -name "hello.cc" -type f -mmin -2 2>/dev/null | head -5

echo ""
echo "详细指南: /Users/feifei/agent/neurx-code/UI_SETUP_GUIDE.md"
echo ""
