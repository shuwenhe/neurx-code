#!/bin/bash

echo "======================================"
echo "neurx-code Agent 实时测试"
echo "======================================"

# 清理旧文件
echo "[1] 清理测试环境..."
rm -f /Users/feifei/agent/neurx-code/src/hello.cc
rm -f /tmp/neurx-test.log
echo "  ✅ 删除旧的hello.cc"

# 创建测试工作空间
TEST_WS="/tmp/neurx-test-workspace"
mkdir -p "$TEST_WS"
echo "  ✅ 创建测试工作空间: $TEST_WS"

# 检查进程
PID=$(ps aux | grep neurx-codeApp | grep -v grep | awk '{print $2}')
if [ -z "$PID" ]; then
    echo "  ❌ neurx-codeApp未运行，正在启动..."
    cd /Users/feifei/agent/neurx-code
    ./build/Qt_6_10_3_for_macOS-Debug/neurx-codeApp.app/Contents/MacOS/neurx-codeApp 2>&1 | tee /tmp/neurx-test.log &
    echo "  ✅ 已启动，日志: /tmp/neurx-test.log"
    echo ""
    echo "  等待5秒让应用启动..."
    sleep 5
else
    echo "  ⚠️  neurx-codeApp已在运行 (PID: $PID)"
    echo "  为了捕获完整日志，建议："
    echo "    1. 停止当前应用"
    echo "    2. 重新运行此脚本"
    echo ""
    read -p "  是否继续当前测试？(y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 0
    fi
fi

echo ""
echo "[2] 测试说明"
echo "======================================"
echo ""
echo "在neurx-code UI中执行以下步骤："
echo ""
echo "步骤1: 设置工作空间"
echo "  → 将工作空间路径设置为: $TEST_WS"
echo ""
echo "步骤2: 启用Auto-approve"
echo "  → Settings → Safety → 勾选 'Auto-approve safe tools'"
echo ""
echo "步骤3: 发送测试命令"
echo "  → 在聊天框输入: 创建hello.cc文件，写入Hello World C++程序"
echo ""
echo "步骤4: 观察结果"
echo "  → 应该看到Agent生成代码并自动创建文件"
echo ""

# 如果有日志文件，设置监控
if [ -f /tmp/neurx-test.log ]; then
    echo "[3] 实时日志监控"
    echo "======================================"
    echo "在另一个终端运行此命令查看实时日志："
    echo "  tail -f /tmp/neurx-test.log | grep --color=always -E 'AgentFileWriterTool|agent_file_writer|tool|error'"
    echo ""
fi

echo "[4] 验证文件创建"
echo "======================================"
echo "执行命令后，运行此命令检查："
echo "  ls -lh $TEST_WS/hello.cc && cat $TEST_WS/hello.cc"
echo ""

echo "等待测试完成..."
echo "按Ctrl+C结束监控"
echo ""

# 监控文件创建
echo "监控 $TEST_WS 目录..."
while true; do
    if [ -f "$TEST_WS/hello.cc" ]; then
        echo ""
        echo "✅ ✅ ✅ 成功！hello.cc已创建！"
        ls -lh "$TEST_WS/hello.cc"
        echo ""
        echo "--- 文件内容 ---"
        cat "$TEST_WS/hello.cc"
        echo ""
        echo "--- 文件内容结束 ---"
        break
    fi
    sleep 1
    echo -n "."
done

echo ""
echo "======================================"
echo "测试完成！"
echo "======================================"
