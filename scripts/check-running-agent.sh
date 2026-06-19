#!/bin/bash

echo "======================================"
echo "neurx-code Agent 运行状态诊断"
echo "======================================"

# 1. 检查进程
echo -e "\n[1] 检查运行的进程..."
PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')
if [ -n "$PID" ]; then
    echo "  ✅ neurx-codeApp正在运行 (PID: $PID)"
    BINARY=$(ps -p $PID -o command= | awk '{print $1}')
    echo "  📍 二进制位置: $BINARY"
    
    # 检查编译时间
    if [ -f "$BINARY" ]; then
        MOD_TIME=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$BINARY")
        SIZE=$(ls -lh "$BINARY" | awk '{print $5}')
        echo "  📅 编译时间: $MOD_TIME"
        echo "  📦 文件大小: $SIZE"
    fi
    
    # 检查是否包含修复
    if strings "$BINARY" | grep -q "No operation specified, defaulting"; then
        echo "  ✅ 包含参数兼容性修复"
    else
        echo "  ❌ 不包含参数兼容性修复"
    fi
else
    echo "  ❌ neurx-codeApp未运行"
    exit 1
fi

# 2. 检查设置
echo -e "\n[2] 需要确认的设置..."
echo "  请在neurx-code UI中检查："
echo "  1. 工作空间路径是否已设置？"
echo "  2. Settings → Safety → 'Auto-approve safe tools' 是否启用？"
echo "  3. Provider是否支持工具调用？（OpenAI/Anthropic/Gemini）"

# 3. 测试提示
echo -e "\n[3] 建议的测试步骤..."
echo "  1. 在neurx-code中发送命令:"
echo "     \"创建hello.cc文件，内容为Hello World程序\""
echo ""
echo "  2. 观察UI中的响应"
echo "     应该看到: 工具调用和文件创建成功的消息"
echo ""
echo "  3. 检查工作空间目录"
echo "     ls -lh <工作空间路径>/hello.cc"

# 4. 捕获日志的方法
echo -e "\n[4] 如果问题仍存在，捕获详细日志..."
echo "  步骤A: 停止当前运行的neurx-code"
echo "  步骤B: 从终端启动并捕获日志:"
echo "    cd /Users/feifei/agent/neurx-code"
echo "    ./build/Qt_6_10_3_for_macOS-Debug/neurx-codeApp.app/Contents/MacOS/neurx-codeApp 2>&1 | tee /tmp/neurx-full.log"
echo ""
echo "  步骤C: 在另一个终端实时查看日志:"
echo "    tail -f /tmp/neurx-full.log | grep -E 'AgentFileWriterTool|tool result|toolCalls'"
echo ""
echo "  步骤D: 发送测试命令后，查看完整日志:"
echo "    cat /tmp/neurx-full.log | grep -A5 -B5 'agent_file_writer'"

# 5. 常见问题
echo -e "\n[5] 常见问题排查..."
echo "  问题A: LLM没有调用工具"
echo "    → 切换到支持工具调用的模型"
echo "    → 使用更明确的提示词"
echo ""
echo "  问题B: 工具被调用但失败"
echo "    → 查看日志中的错误信息"
echo "    → 检查工作空间路径权限"
echo ""
echo "  问题C: 需要手动批准"
echo "    → 确认 auto-approve 已启用"
echo "    → 检查可执行文件是最新版本"

echo -e "\n======================================"
echo "诊断完成"
echo "======================================"
