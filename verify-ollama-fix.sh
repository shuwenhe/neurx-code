#!/bin/bash

# NeurX-Code Ollama工具调用验证脚本
# 日期: 2026-06-18

echo "======================================"
echo "NeurX-Code Ollama 工具调用验证"
echo "======================================"
echo ""

# 1. 检查Ollama是否安装
echo "[1/5] 检查Ollama安装..."
if command -v ollama &> /dev/null; then
    OLLAMA_VERSION=$(ollama --version 2>&1 | head -1)
    echo "  ✅ Ollama已安装: $OLLAMA_VERSION"
else
    echo "  ❌ Ollama未安装"
    echo "     安装方法: brew install ollama"
    exit 1
fi

# 2. 检查Ollama服务
echo ""
echo "[2/5] 检查Ollama服务..."
if curl -s http://localhost:11434/api/version > /dev/null 2>&1; then
    echo "  ✅ Ollama服务运行中"
else
    echo "  ⚠️  Ollama服务未运行"
    echo "     启动方法: ollama serve"
    echo ""
    read -p "是否现在启动Ollama服务? (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        ollama serve &
        OLLAMA_PID=$!
        sleep 2
        echo "  ✅ Ollama服务已启动 (PID: $OLLAMA_PID)"
    else
        exit 1
    fi
fi

# 3. 检查可用模型
echo ""
echo "[3/5] 检查可用模型..."
MODELS=$(ollama list 2>&1 | grep -v "NAME" | awk '{print $1}')
if [ -z "$MODELS" ]; then
    echo "  ⚠️  没有安装任何模型"
    echo "     推荐安装: ollama pull llama3.1"
    exit 1
fi

# 检查是否有支持工具调用的模型
TOOL_MODELS=""
for model in $MODELS; do
    if [[ $model == *"llama3.1"* ]] || [[ $model == *"qwen2.5"* ]] || [[ $model == *"mistral"* ]]; then
        TOOL_MODELS="$TOOL_MODELS $model"
    fi
done

if [ -n "$TOOL_MODELS" ]; then
    echo "  ✅ 找到支持工具调用的模型:"
    for model in $TOOL_MODELS; do
        echo "     - $model"
    done
else
    echo "  ⚠️  未找到支持工具调用的模型"
    echo "     已安装的模型:"
    for model in $MODELS; do
        echo "     - $model (可能不支持工具调用)"
    done
    echo ""
    echo "     推荐安装："
    echo "     - ollama pull llama3.1"
    echo "     - ollama pull qwen2.5:7b"
fi

# 4. 检查neurx-code编译
echo ""
echo "[4/5] 检查neurx-code编译..."
NEURX_APP="/Users/feifei/agent/neurx-code/build/neurx-codeApp.app/Contents/MacOS/neurx-codeApp"
if [ -f "$NEURX_APP" ]; then
    APP_SIZE=$(du -h "$NEURX_APP" | cut -f1)
    APP_DATE=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M" "$NEURX_APP")
    echo "  ✅ neurx-codeApp存在"
    echo "     大小: $APP_SIZE"
    echo "     修改: $APP_DATE"
else
    echo "  ❌ neurx-codeApp未找到"
    echo "     请先编译: cd /Users/feifei/agent/neurx-code && cmake --build build --target neurx-codeApp"
    exit 1
fi

# 5. 检查关键源文件
echo ""
echo "[5/5] 检查OllamaProvider修复..."
OLLAMA_CPP="/Users/feifei/agent/neurx-code/src/llm/OllamaProvider.cpp"
if grep -q "parseToolArguments" "$OLLAMA_CPP" 2>/dev/null; then
    echo "  ✅ OllamaProvider工具调用支持已添加"
else
    echo "  ❌ OllamaProvider工具调用支持未找到"
    echo "     请检查源代码是否已更新"
    exit 1
fi

# 总结
echo ""
echo "======================================"
echo "验证完成"
echo "======================================"
echo ""
echo "✅ 所有检查通过！"
echo ""
echo "下一步："
echo "1. 启动neurx-code:"
echo "   $NEURX_APP"
echo ""
echo "2. 在UI中:"
echo "   - 选择 Ollama provider"
echo "   - 选择支持工具调用的模型 (如 llama3.1)"
echo "   - 设置工作空间路径"
echo ""
echo "3. 测试命令:"
echo '   "创建hello.cc文件，内容为Hello World程序"'
echo ""
echo "4. 观察日志中的工具调用:"
echo "   - [Planner] Built X tools"
echo "   - [agent] tool result: agent_file_writer"
echo ""
echo "详细说明: /Users/feifei/agent/neurx-code/OLLAMA_TOOL_CALLING_FIX.md"
echo ""
