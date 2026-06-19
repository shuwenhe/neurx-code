#!/bin/bash

echo "╔════════════════════════════════════════════════════════╗"
echo "║  测试 neurx-code Agent 文件创建能力                   ║"
echo "╚════════════════════════════════════════════════════════╝"
echo ""

# 检查neurx-code是否运行
PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')
if [ -z "$PID" ]; then
    echo "❌ neurx-codeApp未运行"
    exit 1
fi

BINARY=$(ps -p $PID -o command= | awk '{print $1}')
echo "✅ neurx-codeApp运行中"
echo "   PID: $PID"
echo "   编译时间: $(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$BINARY" 2>/dev/null)"

# 检查是否包含修复
if strings "$BINARY" | grep -q "No operation specified, defaulting"; then
    echo "✅ 包含参数兼容性修复"
else
    echo "❌ 不包含修复，需要重新编译"
    exit 1
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "现在请在 neurx-code UI 中完成以下操作:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "1️⃣  设置工作空间路径"
echo "    → 将工作空间设置为: /Users/feifei/agent/neurx-code"
echo "    → 检查窗口标题应显示此路径"
echo ""
echo "2️⃣  启用 Auto-approve"
echo "    → 点击右上角 Settings（⚙️）"
echo "    → 滚动到 Safety 部分"
echo "    → 打开 'Auto-approve safe tools' 开关"
echo ""
echo "3️⃣  在聊天框输入命令"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "在 neurx-code 的聊天输入框中输入:"
echo ""
echo "  在src目录创建quick_sort.cc文件，实现C++快速排序算法"
echo ""
echo "或者更详细的:"
echo ""
echo "  创建src/quick_sort.cc文件，用C++实现快速排序，"
echo "  包含完整的测试用例和main函数"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# 目标文件
TARGET="/Users/feifei/agent/neurx-code/src/quick_sort.cc"

# 清理旧文件
[ -f "$TARGET" ] && rm -f "$TARGET"

echo "等待文件创建... (监控60秒)"
echo "目标: $TARGET"
echo ""

# 监控
COUNT=0
while [ $COUNT -lt 60 ]; do
    if [ -f "$TARGET" ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🎉 成功！文件已创建！"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        ls -lh "$TARGET"
        echo ""
        echo "━━━━ 前30行预览 ━━━━"
        head -30 "$TARGET"
        echo ""
        if [ $(wc -l < "$TARGET") -gt 30 ]; then
            echo "... (文件还有 $(($(wc -l < "$TARGET") - 30)) 行)"
        fi
        echo ""
        echo "✅ neurx-code Agent 文件创建功能正常工作！"
        echo ""
        echo "查看完整文件: cat $TARGET"
        echo "编译测试: cd /Users/feifei/agent/neurx-code/src && g++ -std=c++17 quick_sort.cc -o quick_sort"
        echo ""
        exit 0
    fi
    
    [ $((COUNT % 10)) -eq 0 ] && echo -n "等待 ${COUNT}s... "
    echo -n "."
    sleep 1
    COUNT=$((COUNT + 1))
done

echo ""
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "❌ 60秒内未检测到文件创建"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "请确认："
echo ""
echo "✓ 工作空间路径是否设置为: /Users/feifei/agent/neurx-code"
echo "  （查看窗口标题）"
echo ""
echo "✓ Auto-approve 是否已启用"
echo "  （Settings → Safety → Auto-approve safe tools = ON）"
echo ""
echo "✓ 是否在聊天框发送了命令"
echo ""
echo "✓ Agent 是否有响应"
echo "  如果只返回代码文本但没创建文件 → 上述设置有问题"
echo "  如果完全没响应 → 检查Provider和API key"
echo ""
echo "查找可能在其他位置创建的文件:"
find /Users/feifei/agent/neurx-code -name "quick_sort.cc" -type f -mmin -2 2>/dev/null
echo ""
