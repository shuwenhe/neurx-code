#!/bin/bash

# neurx-code 文件写入诊断脚本
# 深度分析为什么agent不能写入文件

echo "=========================================="
echo "neurx-code 文件写入深度诊断"
echo "=========================================="
echo ""

cd /Users/feifei/agent/neurx-code

# 1. 检查关键代码修复
echo "[1] 检查代码修复状态..."
echo ""

# 检查agent_file_writer风险降级
if grep -A5 'if (name == QStringLiteral("agent_file_writer"))' src/bridge/AgentController.cpp | grep -q 'return QStringLiteral("medium")'; then
    echo "  ✅ agent_file_writer风险级别 = medium"
else
    echo "  ❌ agent_file_writer风险级别未修改为medium"
    echo "     检查: src/bridge/AgentController.cpp:2617"
fi

# 检查批准逻辑
if grep -q 'if (m_autoApproveTools && risk != QStringLiteral("high") && risk != QStringLiteral("critical"))' src/bridge/AgentController.cpp; then
    echo "  ✅ 批准逻辑已改进（medium自动批准）"
else
    echo "  ❌ 批准逻辑未改进"
fi

# 检查工具注册
if grep -q 'new AgentFileWriterTool(path, m_registry)' src/bridge/AgentController.cpp; then
    echo "  ✅ AgentFileWriterTool已注册"
else
    echo "  ❌ AgentFileWriterTool未注册"
fi

echo ""
echo "[2] 检查可能的问题..."
echo ""

# 问题0: Ollama是否真的拿到tools
echo "  Ollama tools下发检查:"
if grep -q 'providerId == "openai" || providerId == "ollama"' src/agent/Planner.cpp; then
    echo "    ✅ Planner会给Ollama下发OpenAI兼容tools schema"
else
    echo "    ❌ Planner没有给Ollama下发tools schema"
    echo "       症状: 日志里会出现 'provider=ollama ... tools=0'"
fi
echo ""

# 问题1: 工具名称不匹配
echo "  工具名称检查:"
TOOL_NAME=$(grep -A2 'QString name() const override' src/tools/AgentFileWriterTool.h | grep return | sed 's/.*return "\(.*\)".*/\1/')
echo "    - AgentFileWriterTool::name() = \"$TOOL_NAME\""
echo ""

# 问题2: 参数schema
echo "  参数Schema检查:"
if grep -q '"operation"' src/tools/AgentFileWriterTool.cpp; then
    echo "    ✅ 需要'operation'参数"
    echo "    ✅ 支持的操作: write_single, write_batch, update_file"
else
    echo "    ❌ 参数schema可能有问题"
fi
echo ""

# 问题3: 工具执行逻辑
echo "  工具执行逻辑检查:"
if grep -q 'if (operation == "write_single")' src/tools/AgentFileWriterTool.cpp; then
    echo "    ✅ write_single操作已实现"
else
    echo "    ❌ write_single操作未实现"
fi
echo ""

echo "[3] 生成测试配置..."
echo ""

# 创建测试工作空间
TEST_WS="/tmp/neurx-write-test"
mkdir -p "$TEST_WS"
echo "  ✅ 测试工作空间: $TEST_WS"

# 创建测试提示词文件
cat > /tmp/neurx-test-prompt.txt << 'EOF'
创建hello.cc文件，内容为：
#include <iostream>

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
EOF

echo "  ✅ 测试提示词: /tmp/neurx-test-prompt.txt"
echo ""

echo "[4] 可能的问题分析..."
echo ""

echo "  问题A: LLM生成的参数格式不对"
echo "    → LLM可能生成："
echo '       { "path": "hello.cc", "content": "..." }'
echo "    → 但工具需要："
echo '       { "operation": "write_single", "path": "hello.cc", "content": "..." }'
echo ""

echo "  问题B: 工作空间路径问题"
echo "    → 必须先设置工作空间路径"
echo "    → 文件路径是相对于工作空间的"
echo "    → 现在也支持工作空间内的绝对路径"
echo ""

echo "  问题C: 批准对话框被忽略"
echo "    → 即使降级为medium，可能还有其他批准逻辑"
echo "    → 默认策略已改为Never，安全写入应走风险策略自动放行"
echo "    → 仍会对 .git/.env/.codex 等保护资源请求批准"
echo ""

echo "  问题D: 工具schema未正确暴露给LLM"
echo "    → 检查toOpenAISchema()是否正确生成"
echo "    → 检查parametersSchema()是否完整"
echo ""

echo "  问题E: 绝对路径被writer拒绝"
echo "    → 如果tool result包含'Path traversal detected'"
echo "    → 检查传给agent_file_writer的是不是workspace外路径"
echo ""

echo "[5] 调试步骤..."
echo ""

echo "  步骤1: 启动neurx-code并查看日志"
echo "    ./build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp 2>&1 | tee /tmp/neurx-debug.log"
echo ""

echo "  步骤2: 在日志中查找关键信息"
echo "    grep -E 'agent_file_writer|Registering tool|Built.*tools|tools=|toolCalls=|Path traversal' /tmp/neurx-debug.log"
echo ""

echo "  步骤3: 检查工具调用JSON"
echo "    查看Agent生成的tool_call是否包含正确的参数"
echo ""

echo "  步骤4: 检查工具执行结果"
echo "    grep 'tool result:' /tmp/neurx-debug.log"
echo ""

echo "[6] 快速测试..."
echo ""

# 检查编译状态
if [ -f "build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp" ]; then
    APP_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp")
    echo "  ✅ neurx-codeApp已编译 ($APP_DATE)"
    
    # 检查是否需要重新编译
    SRC_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "src/bridge/AgentController.cpp")
    if [[ "$SRC_DATE" > "$APP_DATE" ]]; then
        echo "  ⚠️  源代码比可执行文件新，可能需要重新编译"
        echo "     源代码: $SRC_DATE"
        echo "     可执行: $APP_DATE"
    fi
else
    echo "  ❌ neurx-codeApp未编译"
fi

echo ""
echo "=========================================="
echo "建议的修复方案"
echo "=========================================="
echo ""

echo "方案1: 优先确认Ollama请求里tools不为0"
echo "  查看日志: [agent] request start: provider=ollama ... tools=N"
echo "  如果N=0，说明Planner没有把tools传给Ollama"
echo ""

echo "方案2: 关注writer的path格式"
echo "  允许工作区内绝对路径，继续拦截workspace外路径"
echo "  如果失败，优先检查tool result里的报错内容"
echo ""

echo "方案3: 改进系统提示词"
echo "  明确告诉LLM优先使用src/hello.cc这类workspace内路径"
echo "  提供正确的参数格式示例"
echo ""

echo "方案4: 添加调试日志"
echo "  在AgentFileWriterTool::execute()中添加详细日志"
echo "  记录收到的参数和执行结果"
echo ""

echo "方案5: 使用更简单的工具"
echo "  检查是否有其他文件写入工具"
echo "  例如: Write, file_system, codex_file_system"
echo ""

echo "=========================================="
echo "立即执行的操作"
echo "=========================================="
echo ""

read -p "是否生成诊断提示? (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    cat <<'EOF'
关键日志检查:
  1. [agent] request start: provider=ollama ... tools=N
  2. [agent] response received: ... toolCalls=M
  3. [agent] tool result: agent_file_writer ...

判读:
  - tools=0     -> Planner没有把tools传给provider
  - toolCalls=0 -> 模型/接口没有实际产出tool call
  - Path traversal detected -> 写入路径超出workspace，或参数path不对
EOF
fi

echo ""
echo "=========================================="
echo "诊断完成"
echo "=========================================="
echo ""
