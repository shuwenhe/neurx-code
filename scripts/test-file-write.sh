#!/bin/bash

# neurx-code 文件写入测试脚本
# 验证agent_file_writer工具是否正常工作

echo "=========================================="
echo "neurx-code 文件写入测试"
echo "=========================================="
echo ""

# 设置测试工作空间
TEST_WORKSPACE="/tmp/neurx-file-write-test"
mkdir -p "$TEST_WORKSPACE"

echo "📁 测试工作空间: $TEST_WORKSPACE"
echo ""

# 检查neurx-code可执行文件
NEURX_APP="/Users/feifei/agent/neurx-code/build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp"
if [ ! -f "$NEURX_APP" ]; then
    echo "❌ neurx-codeApp未找到"
    echo "   请先编译: cd /Users/feifei/agent/neurx-code && cmake --build build --target neurx-codeApp"
    exit 1
fi

APP_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$NEURX_APP")
echo "✅ neurx-codeApp已就绪"
echo "   编译时间: $APP_DATE"
echo ""

# 检查修复是否应用
echo "🔍 检查修复状态..."
echo ""

# 检查1: Ollama是否真的拿到tools schema
if grep -q 'providerId == "openai" || providerId == "ollama"' "/Users/feifei/agent/neurx-code/src/agent/Planner.cpp" 2>/dev/null; then
    echo "  ✅ Ollama会收到OpenAI兼容tools schema"
else
    echo "  ❌ Planner没有给Ollama下发tools schema"
fi

# 检查2: agent_file_writer风险降级
if grep -q 'if (name == QStringLiteral("agent_file_writer"))' "/Users/feifei/agent/neurx-code/src/bridge/AgentController.cpp" 2>/dev/null; then
    echo "  ✅ agent_file_writer风险级别已调整"
else
    echo "  ❌ agent_file_writer风险级别未修改"
fi

# 检查3: agent_file_writer是否接受workspace内绝对路径
if grep -q 'absolute path inside the workspace' "/Users/feifei/agent/neurx-code/src/tools/AgentFileWriterTool.cpp" 2>/dev/null; then
    echo "  ✅ agent_file_writer支持workspace内绝对路径"
else
    echo "  ❌ agent_file_writer仍可能拒绝绝对路径"
fi

echo ""
echo "=========================================="
echo "使用说明"
echo "=========================================="
echo ""
echo "1. 启动neurx-code:"
echo "   $NEURX_APP"
echo ""
echo "2. 首次设置:"
echo "   a) 点击右上角的设置图标"
echo "   b) 找到 'Safety' 部分"
echo "   c) 启用 'Auto-approve safe tools' 开关"
echo "   d) 设置工作空间路径: $TEST_WORKSPACE"
echo ""
echo "3. 选择Provider和Model:"
echo "   推荐组合:"
echo "   - OpenAI + GPT-4"
echo "   - Anthropic + Claude-3.5-Sonnet"
echo "   - Ollama + llama3.1 (需先安装)"
echo ""
echo "4. 测试命令 (在Chat中输入):"
echo '   "创建hello.cc文件，内容为：'
echo '   #include <iostream>'
echo '   int main() {'
echo '       std::cout << \"Hello, World!\" << std::endl;'
echo '       return 0;'
echo '   }"'
echo ""
echo "5. 验证结果:"
echo "   ls -lh $TEST_WORKSPACE/hello.cc"
echo "   cat $TEST_WORKSPACE/hello.cc"
echo ""
echo "=========================================="
echo "预期行为"
echo "=========================================="
echo ""
echo "修复前:"
echo "  - Agent生成文本描述（Ollama）"
echo "  - 或需要手动批准（所有Provider）"
echo "  - 文件不会被创建"
echo ""
echo "修复后:"
echo "  - Agent生成工具调用"
echo "  - Ollama请求日志中 tools 不再是 0"
echo "  - 如果启用auto-approve: 自动执行"
echo "  - 文件被成功创建"
echo "  - 日志显示: [agent] tool result: agent_file_writer"
echo ""
echo "=========================================="
echo "故障排除"
echo "=========================================="
echo ""
echo "问题1: 没有生成工具调用（只有文本）"
echo "  → 模型不支持工具调用"
echo "  → 或日志中 provider=ollama 但 tools=0"
echo "  → 尝试: llama3.1, qwen2.5:7b, GPT-4, Claude"
echo ""
echo "问题2: 显示批准对话框"
echo "  → auto-approve未启用"
echo "  → 解决: 在Settings中启用，或手动点击Approve"
echo ""
echo "问题3: 文件创建失败"
echo "  → 检查工作空间路径是否设置"
echo "  → 检查文件权限"
echo "  → 检查日志里是否出现 Path traversal detected"
echo "  → 查看日志输出"
echo ""
echo "问题4: Ollama连接失败"
echo "  → 启动Ollama: ollama serve"
echo "  → 检查端口: curl http://localhost:11434"
echo ""
echo "=========================================="
echo "快速启动"
echo "=========================================="
echo ""
read -p "是否现在启动neurx-code? (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "正在启动neurx-code..."
    echo ""
    "$NEURX_APP" &
    echo "neurx-code已在后台启动"
    echo "请按照上述步骤配置并测试"
fi

echo ""
echo "=========================================="
echo "更多信息"
echo "=========================================="
echo ""
echo "完整指南: /Users/feifei/agent/neurx-code/COMPLETE_FIX_GUIDE.md"
echo "批准诊断: /Users/feifei/agent/neurx-code/APPROVAL_DIAGNOSIS.md"
echo "Ollama修复: /Users/feifei/agent/neurx-code/OLLAMA_TOOL_CALLING_FIX.md"
echo ""
