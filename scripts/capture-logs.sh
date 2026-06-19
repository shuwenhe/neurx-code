#!/bin/bash

echo "======================================"
echo "捕获neurx-code Agent日志"
echo "======================================"

# 检查是否有日志输出重定向
PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')

if [ -z "$PID" ]; then
    echo "❌ neurx-codeApp未运行"
    exit 1
fi

echo "✅ neurx-codeApp运行中 (PID: $PID)"
echo ""
echo "方法1: 查看系统日志（macOS）"
echo "================================"
echo "从系统日志中提取neurx-code的输出..."
echo ""

# 使用log show查看最近的日志
log show --predicate 'process == "neurx-codeApp"' --style compact --info --debug --last 5m 2>/dev/null | \
    grep -E "AgentFileWriterTool|tool result|Built.*tools|agent_file_writer|Registering tool" | \
    tail -30

echo ""
echo "方法2: 建议重启并捕获完整日志"
echo "================================"
echo ""
echo "为了获得最详细的诊断信息，请执行："
echo ""
echo "1. 停止当前的neurx-code应用"
echo ""
echo "2. 在终端运行（会显示所有日志）："
echo "   cd /Users/feifei/agent/neurx-code"
echo "   ./build/Qt_6_10_3_for_macOS-Debug/neurx-codeApp.app/Contents/MacOS/neurx-codeApp 2>&1 | tee /tmp/neurx-full.log &"
echo ""
echo "3. 在另一个终端实时监控："
echo "   tail -f /tmp/neurx-full.log | grep --color=always -E 'AgentFileWriterTool|agent_file_writer|tool result|error|Error'"
echo ""
echo "4. 发送测试命令: '创建hello.cc文件，内容为Hello World程序'"
echo ""
echo "5. 观察日志输出"
echo ""

echo "方法3: 检查UI状态"
echo "================================"
echo "在neurx-code UI中："
echo "  1. 点击 Settings 图标"
echo "  2. 查找 'Safety' 或 '安全' 选项"
echo "  3. 确认 'Auto-approve safe tools' 是否勾选"
echo "  4. 查看当前工作空间路径显示"
echo ""

echo "方法4: 快速检查文件"
echo "================================"
echo "检查hello.cc是否可能在其他位置被创建："
echo ""
find /Users/feifei/agent/neurx-code -name "hello.cc" -type f 2>/dev/null | head -5
if [ $? -eq 0 ]; then
    echo "  找到hello.cc文件！"
else
    echo "  未找到hello.cc文件"
fi

echo ""
echo "======================================"
