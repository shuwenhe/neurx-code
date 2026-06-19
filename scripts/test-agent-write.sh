#!/bin/bash

cat << 'EOF'
╔═══════════════════════════════════════════════════════════╗
║   测试 neurx-code Agent 创建 quick_sort.cc 文件          ║
╚═══════════════════════════════════════════════════════════╝

EOF

# 检查neurx-code是否运行
PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')
if [ -z "$PID" ]; then
    echo "❌ neurx-codeApp未运行"
    echo ""
    echo "请先启动 neurx-code："
    echo "  /Users/feifei/agent/neurx-code/build/Qt_6_10_3_for_macOS-Debug/neurx-codeApp.app/Contents/MacOS/neurx-codeApp"
    exit 1
fi

echo "✅ neurx-codeApp运行中 (PID: $PID)"
echo ""

# 目标文件
TARGET_FILE="/Users/feifei/agent/neurx-code/src/quick_sort.cc"

# 清理旧文件
if [ -f "$TARGET_FILE" ]; then
    echo "🗑️  删除旧的 quick_sort.cc"
    rm -f "$TARGET_FILE"
fi

cat << 'EOF'
测试步骤
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

步骤1: 在 neurx-code UI 中设置
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  1️⃣  设置工作空间路径为: /Users/feifei/agent/neurx-code
  2️⃣  Settings → Safety → 启用 "Auto-approve safe tools"

步骤2: 在聊天框输入以下命令
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

在 neurx-code 的聊天框中输入:

    在src目录下创建quick_sort.cc文件，实现C++快速排序算法，
    包含完整的函数实现、测试用例和注释

或者更简洁的命令:

    创建src/quick_sort.cc文件，用C++实现快速排序

步骤3: 等待文件创建
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
EOF

echo ""
echo "监控文件创建（60秒）..."
echo "目标文件: $TARGET_FILE"
echo ""

# 监控60秒
COUNT=0
while [ $COUNT -lt 60 ]; do
    if [ -f "$TARGET_FILE" ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🎉 成功！quick_sort.cc 已创建！"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        ls -lh "$TARGET_FILE"
        echo ""
        
        # 显示文件内容（前50行）
        LINE_COUNT=$(wc -l < "$TARGET_FILE")
        echo "━━━━━━ 文件内容 (共 $LINE_COUNT 行) ━━━━━━"
        head -50 "$TARGET_FILE"
        
        if [ $LINE_COUNT -gt 50 ]; then
            echo ""
            echo "... (省略剩余 $((LINE_COUNT - 50)) 行)"
        fi
        
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "✅ 测试成功！Agent文件写入功能正常！"
        echo ""
        echo "查看完整文件:"
        echo "  cat $TARGET_FILE"
        echo ""
        echo "编译测试:"
        echo "  cd /Users/feifei/agent/neurx-code/src"
        echo "  g++ -std=c++17 quick_sort.cc -o quick_sort"
        echo "  ./quick_sort"
        echo ""
        exit 0
    fi
    
    # 每5秒显示一个提示
    if [ $((COUNT % 5)) -eq 0 ]; then
        echo -n "等待中... (${COUNT}s) "
    fi
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
echo "问题排查:"
echo ""
echo "1. ⚠️  工作空间路径是否设置正确？"
echo "   当前应该设置为: /Users/feifei/agent/neurx-code"
echo "   检查窗口标题是否显示此路径"
echo ""
echo "2. ⚠️  Auto-approve 是否启用？"
echo "   Settings → Safety → 'Auto-approve safe tools' = ON"
echo ""
echo "3. ⚠️  是否发送了命令？"
echo "   在聊天框输入创建文件的命令"
echo ""
echo "4. 🔍 查看Agent的响应"
echo "   如果只返回代码文本但没创建文件，说明："
echo "   - 工作空间路径未设置"
echo "   - 或 Auto-approve 未启用"
echo ""
echo "5. 🔍 检查是否在其他位置创建了文件"
find /Users/feifei/agent/neurx-code -name "quick_sort.cc" -type f -mmin -2 2>/dev/null

echo ""
echo "详细诊断:"
echo "  /Users/feifei/agent/neurx-code/check-running-agent.sh"
echo ""
