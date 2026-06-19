#!/bin/bash

echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║          neurx-code UI配置诊断工具                          ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo ""

echo "📋 请回答以下问题来诊断问题："
echo ""

# 问题1：是否发送了命令
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "❓ 问题1：你是否在neurx-code的聊天框中发送了命令？"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  [1] 是，我发送了命令"
echo "  [2] 否，我还没发送"
echo ""
read -p "请输入数字 [1或2]: " sent_command
echo ""

if [ "$sent_command" = "2" ]; then
    echo "💡 请先在neurx-code聊天框中输入命令："
    echo ""
    echo "    创建src/hello_agent.cc文件，写一个简单的Hello World C++程序"
    echo ""
    echo "发送后，重新运行测试脚本。"
    exit 0
fi

# 问题2：Agent是否有响应
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "❓ 问题2：Agent有响应吗？"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "  [1] 有响应，显示了代码"
echo "  [2] 完全没有响应"
echo "  [3] 显示错误信息"
echo ""
read -p "请输入数字 [1/2/3]: " response_type
echo ""

case $response_type in
    1)
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🔍 诊断：Agent返回了代码但没有创建文件"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "📌 这是配置问题！请检查："
        echo ""
        echo "✅ 步骤1：设置工作空间路径"
        echo "   → 在neurx-code窗口标题查看当前路径"
        echo "   → 如果不是 /Users/feifei/agent/neurx-code"
        echo "   → 点击 File → Open Workspace"
        echo "   → 选择: /Users/feifei/agent/neurx-code"
        echo ""
        echo "✅ 步骤2：启用Auto-approve"
        echo "   → 点击右上角 ⚙️ Settings"
        echo "   → 找到 Safety 部分"
        echo "   → 确认 'Auto-approve safe tools' 开关是 ON"
        echo ""
        echo "✅ 步骤3：Agent响应中查看"
        echo "   → 滚动查看Agent的完整回复"
        echo "   → 看是否有提到 'agent_file_writer' 工具"
        echo "   → 如果提到了但说需要'批准' → auto-approve未生效"
        echo ""
        echo "💡 配置后，再次发送命令测试"
        ;;
        
    2)
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🔍 诊断：Agent完全没响应"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        echo "📌 这是Provider配置问题！请检查："
        echo ""
        echo "✅ 检查1：Provider是否配置"
        echo "   → 点击右上角 ⚙️ Settings"
        echo "   → 找到 LLM Provider 部分"
        echo "   → 确认选择了 OpenAI 或 Ollama"
        echo ""
        echo "✅ 检查2：API Key配置（如果用OpenAI）"
        echo "   → Settings → OpenAI 部分"
        echo "   → 确认填写了API Key"
        echo "   → 确认Endpoint是: https://api.siliconflow.cn/v1/chat/completions"
        echo "   → 确认Model是: Qwen/Qwen3-32B"
        echo ""
        echo "✅ 检查3：Ollama服务（如果用Ollama）"
        echo "   → 确认Ollama正在运行"
        echo "   → 测试: curl http://localhost:11434/api/tags"
        echo ""
        ;;
        
    3)
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo "🔍 诊断：Agent显示错误"
        echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
        echo ""
        read -p "请输入错误信息（或按回车跳过）: " error_msg
        echo ""
        if [ -n "$error_msg" ]; then
            echo "📝 错误信息: $error_msg"
            echo ""
        fi
        echo "💡 常见错误解决方案："
        echo ""
        echo "• 'API key invalid' → 重新配置API key"
        echo "• 'Connection refused' → 检查网络或Endpoint"
        echo "• 'Model not found' → 检查Model名称是否正确"
        echo "• 'Permission denied' → 检查文件权限"
        echo ""
        ;;
esac

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 快速验证配置"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "在neurx-code聊天框输入简单命令测试："
echo ""
echo "  你好，请自我介绍"
echo ""
echo "如果这个命令都没响应 → Provider配置问题"
echo "如果有响应但创建文件失败 → workspace/auto-approve配置问题"
echo ""
