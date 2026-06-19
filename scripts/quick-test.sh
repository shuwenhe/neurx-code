#!/bin/bash

cat << 'EOF'
╔═══════════════════════════════════════════════════════════╗
║          neurx-code Agent 一键测试指南                    ║
╚═══════════════════════════════════════════════════════════╝

当前状态检查
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
EOF

# 检查进程
PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')
if [ -n "$PID" ]; then
    BINARY=$(ps -p $PID -o command= | awk '{print $1}')
    MOD_TIME=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$BINARY" 2>/dev/null)
    echo "✅ neurx-codeApp运行中 (PID: $PID)"
    echo "   编译时间: $MOD_TIME"
    
    # 检查修复
    if strings "$BINARY" | grep -q "No operation specified, defaulting"; then
        echo "✅ 包含所有修复"
    else
        echo "❌ 不包含修复 - 需要重新编译"
        exit 1
    fi
else
    echo "❌ neurx-codeApp未运行"
    exit 1
fi

cat << 'EOF'

快速测试步骤
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

步骤1: 创建测试工作空间
EOF

TEST_WS="/tmp/neurx-test-$(date +%s)"
mkdir -p "$TEST_WS"
echo "   ✅ 已创建: $TEST_WS"

cat << EOF

步骤2: 在neurx-code UI中配置
   1️⃣  设置工作空间路径为: $TEST_WS
   2️⃣  Settings → Safety → 启用 "Auto-approve safe tools"
   3️⃣  确认使用支持工具调用的Provider (OpenAI/Anthropic)

步骤3: 发送测试命令
   📝 在聊天框输入:
   
   创建hello.cc文件，写入Hello World C++程序

步骤4: 查看结果
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

   等待Agent响应后，运行以下命令验证:

   ls -lh $TEST_WS/hello.cc
   cat $TEST_WS/hello.cc

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

监控文件创建（按Ctrl+C停止）
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
EOF

echo "监控目录: $TEST_WS"
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
        echo "✅ 测试成功！Agent文件写入功能正常！"
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
echo "可能的原因:"
echo "  1. 工作空间路径未正确设置为: $TEST_WS"
echo "  2. Auto-approve未启用"
echo "  3. LLM没有生成工具调用"
echo "  4. 工具执行失败"
echo ""
echo "建议操作:"
echo "  1. 检查工作空间路径设置"
echo "  2. 查看UI中Agent的响应"
echo "  3. 运行详细诊断:"
echo "     /Users/feifei/agent/neurx-code/check-running-agent.sh"
echo ""
echo "查找可能被创建在其他位置的文件:"
find /Users/feifei/agent/neurx-code -name "hello.cc" -type f -mmin -2 2>/dev/null | head -5
echo ""
