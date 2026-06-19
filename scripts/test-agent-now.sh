#!/bin/bash

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║          neurx-code Agent 文件创建实时测试                   ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

TARGET="/Users/feifei/agent/neurx-code/src/hello_agent.cc"

# 清理旧文件
[ -f "$TARGET" ] && rm -f "$TARGET"

echo "📋 测试步骤："
echo ""
echo "1️⃣  在neurx-code UI中设置工作空间"
echo "    → 点击 File → Open Workspace (或类似菜单)"
echo "    → 选择: /Users/feifei/agent/neurx-code"
echo "    → 确认窗口标题显示该路径"
echo ""
echo "2️⃣  启用Auto-approve（重要！）"
echo "    → 点击右上角 Settings 图标（⚙️）"
echo "    → 找到 Safety 部分"
echo "    → 打开 'Auto-approve safe tools' 开关"
echo "    → 关闭设置面板"
echo ""
echo "3️⃣  在聊天框输入以下命令："
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  创建src/hello_agent.cc文件，写一个简单的Hello World C++程序"
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "⏱️  准备就绪后，按回车开始监控（60秒）..."
read -r

echo ""
echo "🔍 监控中: $TARGET"
echo ""

COUNT=0
while [ $COUNT -lt 60 ]; do
    if [ -f "$TARGET" ]; then
        echo ""
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🎉🎉🎉 成功！Agent创建了文件！"
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
        echo "✅ neurx-code Agent 文件写入功能正常工作！"
        echo ""
        echo "🧪 编译测试:"
        echo "   cd /Users/feifei/agent/neurx-code/src"
        echo "   g++ -std=c++17 hello_agent.cc -o hello_agent && ./hello_agent"
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
echo "⚠️  60秒内未检测到文件"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "🔍 故障排查："
echo ""
echo "❓ Agent有响应吗？"
echo "   → 如果只显示代码不创建文件 → 检查workspace路径和auto-approve"
echo "   → 如果完全没响应 → 检查Provider配置和API key"
echo ""
echo "❓ 可能在其他位置创建？"
find /Users/feifei/agent/neurx-code -name "hello_agent.cc" -type f -mmin -2 2>/dev/null
echo ""
echo "❓ 检查neurx-code日志:"
echo "   在UI中查看聊天历史，看agent是否调用了agent_file_writer工具"
echo ""
