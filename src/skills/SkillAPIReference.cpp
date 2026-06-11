#include "SkillAPIReference.h"
#include <QDebug>

SkillAPIReference::SkillAPIReference()
{
    initializeModels();
    initializeSDKReferences();
    initializeAPIPatterns();
}

void SkillAPIReference::initializeModels()
{
    m_models = {
        {
            "claude-opus-4-8",
            "Claude Opus 4.8",
            "Most capable model with advanced reasoning",
            200000,
            "2025-01-01",
            3.0,
            15.0,
            "opus",
            {"vision", "tool-use", "streaming", "batch"},
            true
        },
        {
            "claude-sonnet-4-20250514",
            "Claude Sonnet 4",
            "Balanced performance and speed",
            200000,
            "2024-09-01",
            3.0,
            15.0,
            "sonnet",
            {"vision", "tool-use", "streaming", "batch"},
            true
        },
        {
            "claude-haiku-4-5",
            "Claude Haiku 4.5",
            "Fast, cost-effective model",
            8000,
            "2024-03-01",
            0.8,
            4.0,
            "haiku",
            {"tool-use", "streaming"},
            true
        }
    };
}

void SkillAPIReference::initializeSDKReferences()
{
    m_sdkReferences[ProgrammingLanguage::Python] = {
        ProgrammingLanguage::Python,
        "Python",
        "anthropic",
        "https://github.com/anthropics/anthropic-sdk-python",
        "0.35.0",
        {"streaming", "vision", "tool-use", "batch", "caching"},
        "pip install anthropic"
    };

    m_sdkReferences[ProgrammingLanguage::TypeScript] = {
        ProgrammingLanguage::TypeScript,
        "TypeScript",
        "@anthropic-ai/sdk",
        "https://github.com/anthropics/anthropic-sdk-typescript",
        "0.35.0",
        {"streaming", "vision", "tool-use", "batch", "caching"},
        "npm install @anthropic-ai/sdk"
    };

    m_sdkReferences[ProgrammingLanguage::Java] = {
        ProgrammingLanguage::Java,
        "Java",
        "com.anthropic:anthropic-client",
        "https://github.com/anthropics/anthropic-sdk-java",
        "0.4.0",
        {"streaming", "vision", "tool-use"},
        "<!-- Maven --> <dependency><groupId>com.anthropic</groupId><artifactId>anthropic-client</artifactId><version>0.4.0</version></dependency>"
    };

    m_sdkReferences[ProgrammingLanguage::Go] = {
        ProgrammingLanguage::Go,
        "Go",
        "github.com/anthropics/anthropic-sdk-go",
        "https://github.com/anthropics/anthropic-sdk-go",
        "0.2.0",
        {"streaming", "vision", "tool-use"},
        "go get github.com/anthropics/anthropic-sdk-go"
    };

    m_sdkReferences[ProgrammingLanguage::Ruby] = {
        ProgrammingLanguage::Ruby,
        "Ruby",
        "anthropic",
        "https://github.com/anthropics/anthropic-sdk-ruby",
        "0.3.0",
        {"streaming", "vision", "tool-use"},
        "gem install anthropic"
    };

    m_sdkReferences[ProgrammingLanguage::CSharp] = {
        ProgrammingLanguage::CSharp,
        "C#",
        "Anthropic",
        "https://github.com/anthropics/anthropic-sdk-csharp",
        "0.4.0",
        {"streaming", "vision", "tool-use"},
        "dotnet add package Anthropic"
    };

    m_sdkReferences[ProgrammingLanguage::PHP] = {
        ProgrammingLanguage::PHP,
        "PHP",
        "anthropic/sdk",
        "https://github.com/anthropics/anthropic-sdk-php",
        "0.5.0",
        {"streaming", "vision", "tool-use"},
        "composer require anthropic/sdk"
    };

    m_sdkReferences[ProgrammingLanguage::cURL] = {
        ProgrammingLanguage::cURL,
        "cURL",
        "none",
        "https://docs.anthropic.com/en/api/getting-started",
        "2025-01",
        {"all"},
        "curl --version"
    };
}

void SkillAPIReference::initializeAPIPatterns()
{
    m_apiPatterns = {
        {
            "Basic Message",
            "Send a basic message and get a response",
            "core",
            "client.messages.create(model='claude-opus-4-8', max_tokens=1024, messages=[...])",
            {"claude-opus-4-8", "claude-sonnet-4-20250514", "claude-haiku-4-5"}
        },
        {
            "Streaming",
            "Stream responses token-by-token",
            "streaming",
            "with client.messages.stream(...) as stream: for text in stream.text_stream: print(text)",
            {"claude-opus-4-8", "claude-sonnet-4-20250514", "claude-haiku-4-5"}
        },
        {
            "Tool Use",
            "Enable models to use tools/functions",
            "tool-use",
            "response = client.messages.create(tools=[...], model='claude-opus-4-8')",
            {"claude-opus-4-8", "claude-sonnet-4-20250514"}
        },
        {
            "Vision",
            "Process images and analyze visual content",
            "vision",
            "messages=[{\"role\": \"user\", \"content\": [{\"type\": \"image\", \"source\": {...}}]}]",
            {"claude-opus-4-8", "claude-sonnet-4-20250514"}
        },
        {
            "Batch Processing",
            "Process multiple requests efficiently",
            "batch",
            "batch = client.beta.batch.create(requests=[...])",
            {"claude-opus-4-8", "claude-sonnet-4-20250514"}
        },
        {
            "Prompt Caching",
            "Cache prompts for cost savings",
            "caching",
            "system=[{\"type\": \"text\", \"text\": ..., \"cache_control\": {\"type\": \"ephemeral\"}}]",
            {"claude-opus-4-8", "claude-sonnet-4-20250514"}
        }
    };
}

QVector<SkillAPIReference::ModelInfo> SkillAPIReference::getAvailableModels() const
{
    return m_models;
}

SkillAPIReference::ModelInfo SkillAPIReference::getModelInfo(const QString &modelId) const
{
    for (const auto &model : m_models) {
        if (model.modelId == modelId) {
            return model;
        }
    }
    return ModelInfo();
}

QString SkillAPIReference::getLatestRecommendedModel() const
{
    for (const auto &model : m_models) {
        if (model.available && model.family == QStringLiteral("opus")) {
            return model.modelId;
        }
    }

    if (!m_models.isEmpty()) {
        return m_models.first().modelId;
    }

    return {};
}

QString SkillAPIReference::getPricingInfo(const QString &modelId) const
{
    auto model = getModelInfo(modelId);
    if (model.modelId.isEmpty()) {
        return "Model not found";
    }

    return QString(
        "%1 Pricing:\n"
        "  Input: $%2 per million tokens\n"
        "  Output: $%3 per million tokens\n"
        "  Max tokens: %4\n"
        "  Released: %5"
    ).arg(model.displayName)
     .arg(model.pricePerMTok, 0, 'f', 2)
     .arg(model.pricePerOutputMTok, 0, 'f', 2)
     .arg(model.maxTokens)
     .arg(model.releaseDate);
}

SkillAPIReference::SDKReference SkillAPIReference::getSDKReference(
    ProgrammingLanguage language) const
{
    if (m_sdkReferences.contains(language)) {
        return m_sdkReferences[language];
    }
    return SDKReference();
}

QString SkillAPIReference::getInstallCommand(ProgrammingLanguage language) const
{
    auto sdk = getSDKReference(language);
    if (sdk.sdkName.isEmpty()) {
        return "SDK not found for this language";
    }
    return sdk.installCommand;
}

QString SkillAPIReference::getQuickStart(ProgrammingLanguage language) const
{
    const QString modelId = getLatestRecommendedModel();
    const QString langName = languageToString(language);

    switch (language) {
    case ProgrammingLanguage::Python:
        return QStringLiteral(
            "from anthropic import Anthropic\n\n"
            "client = Anthropic()\n"
            "response = client.messages.create(\n"
            "    model=\"%1\",\n"
            "    max_tokens=1024,\n"
            "    messages=[{\"role\": \"user\", \"content\": \"Hello, Claude!\"}]\n"
            ")\n"
            "print(response.content[0].text)\n"
        ).arg(modelId);
    case ProgrammingLanguage::TypeScript:
        return QStringLiteral(
            "import Anthropic from \"@anthropic-ai/sdk\";\n\n"
            "const client = new Anthropic();\n"
            "const response = await client.messages.create({\n"
            "  model: \"%1\",\n"
            "  max_tokens: 1024,\n"
            "  messages: [{ role: \"user\", content: \"Hello, Claude!\" }]\n"
            "});\n"
            "console.log(response.content[0].type === \"text\" ? response.content[0].text : \"\");\n"
        ).arg(modelId);
    case ProgrammingLanguage::Java:
        return QStringLiteral(
            "AnthropicClient client = AnthropicClient.builder()\n"
            "    .apiKey(System.getenv(\"ANTHROPIC_API_KEY\"))\n"
            "    .build();\n"
            "// Send a message to model %1 with max_tokens=1024.\n"
        ).arg(modelId);
    case ProgrammingLanguage::Go:
        return QStringLiteral(
            "client := anthropic.NewClient()\n"
            "resp, err := client.Messages.New(ctx, anthropic.MessageNewParams{\n"
            "    Model: \"%1\",\n"
            "    MaxTokens: 1024,\n"
            "    Messages: []anthropic.MessageParam{{Role: \"user\", Content: \"Hello, Claude!\"}},\n"
            "})\n"
            "_ = resp\n"
            "_ = err\n"
        ).arg(modelId);
    case ProgrammingLanguage::Ruby:
        return QStringLiteral(
            "client = Anthropic::Client.new\n"
            "response = client.messages.create(\n"
            "  model: \"%1\",\n"
            "  max_tokens: 1024,\n"
            "  messages: [{ role: \"user\", content: \"Hello, Claude!\" }]\n"
            ")\n"
            "puts response.content[0].text\n"
        ).arg(modelId);
    case ProgrammingLanguage::CSharp:
        return QStringLiteral(
            "var client = new AnthropicClient();\n"
            "var response = await client.Messages.CreateAsync(new MessageCreateRequest\n"
            "{\n"
            "    Model = \"%1\",\n"
            "    MaxTokens = 1024,\n"
            "    Messages = new[] { new MessageParam(\"user\", \"Hello, Claude!\") }\n"
            "});\n"
        ).arg(modelId);
    case ProgrammingLanguage::PHP:
        return QStringLiteral(
            "$client = new Anthropic\\Client(getenv('ANTHROPIC_API_KEY'));\n"
            "$response = $client->messages()->create([\n"
            "    'model' => '%1',\n"
            "    'max_tokens' => 1024,\n"
            "    'messages' => [ ['role' => 'user', 'content' => 'Hello, Claude!'] ],\n"
            "]);\n"
        ).arg(modelId);
    case ProgrammingLanguage::cURL:
        return QStringLiteral(
            "curl https://api.anthropic.com/v1/messages \\\n"
            "  -H \"x-api-key: $ANTHROPIC_API_KEY\" \\\n"
            "  -H \"anthropic-version: 2023-06-01\" \\\n"
            "  -H \"content-type: application/json\" \\\n"
            "  -d '{\"model\":\"%1\",\"max_tokens\":1024,\"messages\":[{\"role\":\"user\",\"content\":\"Hello, Claude!\"}]}'\n"
        ).arg(modelId);
    }

    return QStringLiteral("Quick start guide for %1 is not available yet.").arg(langName);
}

QVector<SkillAPIReference::APIPattern> SkillAPIReference::getAPIPatterns(
    const QString &category) const
{
    if (category.isEmpty()) {
        return m_apiPatterns;
    }

    QVector<APIPattern> filtered;
    for (const auto &pattern : m_apiPatterns) {
        if (pattern.category == category) {
            filtered.append(pattern);
        }
    }
    return filtered;
}

QString SkillAPIReference::getPatternExample(
    const QString &patternName,
    ProgrammingLanguage language) const
{
    for (const auto &pattern : m_apiPatterns) {
        if (pattern.name == patternName) {
            if (language == ProgrammingLanguage::Python || language == ProgrammingLanguage::TypeScript) {
                return pattern.codeExample;
            }

            return QStringLiteral(
                "%1\n\nLanguage-specific example for %2 is not expanded yet, but the workflow is the same."
            ).arg(pattern.codeExample, languageToString(language));
        }
    }
    return "Pattern not found";
}

QString SkillAPIReference::getMigrationGuide(
    const QString &fromModel,
    const QString &toModel,
    ProgrammingLanguage language) const
{
    return QString(
        "Migration Guide: %1 → %2 (Language: %3)\n\n"
        "This is a migration guide template. Actual guides should be specific to:\n"
        "- API changes between models\n"
        "- New capabilities available in %2\n"
        "- Breaking changes to watch for\n"
        "- Performance differences"
    ).arg(fromModel, toModel, languageToString(language));
}

QString SkillAPIReference::getToolUseReference(ProgrammingLanguage language) const
{
    return QString(
        "Tool Use Reference for %1\n\n"
        "Tool use (also called function calling) enables Claude to interact with external systems.\n\n"
        "Key concepts:\n"
        "1. Define tools with JSON schemas describing parameters\n"
        "2. Include tools in message creation\n"
        "3. Handle tool_use content blocks in responses\n"
        "4. Return tool results\n"
        "5. Let Claude decide when to use tools"
    ).arg(languageToString(language));
}

QString SkillAPIReference::getToolDefinitionExamples() const
{
    return R"(
Example tool definition (JSON Schema):
{
    "name": "get_weather",
    "description": "Get weather forecast for a location",
    "input_schema": {
        "type": "object",
        "properties": {
            "location": {
                "type": "string",
                "description": "City name"
            },
            "days": {
                "type": "integer",
                "description": "Number of days forecast"
            }
        },
        "required": ["location"]
    }
}
    )";
}

QString SkillAPIReference::getStreamingGuide(ProgrammingLanguage language) const
{
    return QString(
        "Streaming Guide for %1\n\n"
        "Streaming allows you to get responses token-by-token, enabling:\n"
        "- Lower perceived latency\n"
        "- Progressive display of results\n"
        "- Processing of large outputs\n\n"
        "Streaming is recommended for:\n"
        "- Long content generation\n"
        "- User-facing applications\n"
        "- Cost-conscious applications (avoids timeout retries)"
    ).arg(languageToString(language));
}

QString SkillAPIReference::getVisionGuide(ProgrammingLanguage language) const
{
    return QString(
        "Vision Capabilities Guide for %1\n\n"
        "Claude can analyze images, charts, diagrams, and screenshots.\n"
        "Supported formats: JPEG, PNG, GIF, WebP\n\n"
        "Use cases:\n"
        "- Document analysis\n"
        "- Chart interpretation\n"
        "- UI analysis\n"
        "- Visual problem solving"
    ).arg(languageToString(language));
}

QString SkillAPIReference::getPromptCachingGuide(ProgrammingLanguage language) const
{
    return QString(
        "Prompt Caching Guide for %1\n\n"
        "Cache prompts to reduce costs and improve latency.\n"
        "Cache hits: 90%% cost reduction for cached tokens\n\n"
        "Best for:\n"
        "- Large system prompts\n"
        "- Document analysis with follow-up questions\n"
        "- Multi-turn conversations with fixed context"
    ).arg(languageToString(language));
}

QString SkillAPIReference::getBatchProcessingGuide(ProgrammingLanguage language) const
{
    return QString(
        "Batch Processing Guide for %1\n\n"
        "Process multiple requests efficiently with:\n"
        "- 50%% cost reduction\n"
        "- 24-hour processing window\n"
        "- Ideal for non-time-sensitive tasks"
    ).arg(languageToString(language));
}

QString SkillAPIReference::getErrorHandlingGuide(ProgrammingLanguage language) const
{
    return QString(
        "Error Handling Guide for %1\n\n"
        "Common errors and handling:\n"
        "- Authentication errors (API key invalid)\n"
        "- Rate limiting (exponential backoff)\n"
        "- Timeout errors (implement retries)\n"
        "- Invalid requests (validate input)"
    ).arg(languageToString(language));
}

QString SkillAPIReference::getBestPractices() const
{
    return R"(
Claude API Best Practices:

1. Choose the right model
   - Use claude-opus-4-8 for complex reasoning
   - Use claude-sonnet-4-20250514 for balanced performance
   - Use claude-haiku-4-5 for speed/cost

2. Optimize prompts
   - Be specific and clear
   - Provide examples (few-shot prompting)
   - Use system messages for consistent behavior

3. Handle errors gracefully
   - Implement exponential backoff
   - Set appropriate timeouts
   - Validate inputs before API calls

4. Manage costs
   - Use prompt caching for repeated contexts
   - Use batch API for non-urgent requests
   - Monitor token usage

5. Security
   - Never expose API keys
   - Use environment variables
   - Validate all inputs from users
    )";
}

QString SkillAPIReference::getRateLimitingGuide() const
{
    return R"(
Rate Limiting and Retry Strategy:

Rate Limits:
- Depends on your plan
- Default: 50 RPM (requests per minute)
- Errors: 429 Too Many Requests

Retry Strategy (Exponential Backoff):
1. Catch RateLimitError
2. Wait: 2^attempt seconds (1s, 2s, 4s, 8s...)
3. Add jitter: ± random(0s, 1s)
4. Max retries: 5-10 attempts

Best Practices:
- Implement retry logic in production
- Use batch API for non-urgent requests
- Monitor rate limit headers
- Plan capacity for peak usage
    )";
}

SkillAPIReference::ProgrammingLanguage SkillAPIReference::detectLanguage(
    const QString &fileContent,
    const QString &fileName) const
{
    const QString lowerFileName = fileName.toLower();

    // Check by file extension
    if (lowerFileName.endsWith(".py")) return ProgrammingLanguage::Python;
    if (lowerFileName.endsWith(".ts") || lowerFileName.endsWith(".tsx")) return ProgrammingLanguage::TypeScript;
    if (lowerFileName.endsWith(".js") || lowerFileName.endsWith(".jsx") || lowerFileName.endsWith(".mjs") || lowerFileName.endsWith(".cjs")) return ProgrammingLanguage::TypeScript;
    if (lowerFileName.endsWith(".java")) return ProgrammingLanguage::Java;
    if (lowerFileName.endsWith(".go")) return ProgrammingLanguage::Go;
    if (lowerFileName.endsWith(".rb")) return ProgrammingLanguage::Ruby;
    if (lowerFileName.endsWith(".cs")) return ProgrammingLanguage::CSharp;
    if (lowerFileName.endsWith(".php")) return ProgrammingLanguage::PHP;
    if (lowerFileName.endsWith(".sh") || lowerFileName.endsWith(".bash")) return ProgrammingLanguage::cURL;

    // Check by import patterns
    if (fileContent.contains("import anthropic", Qt::CaseInsensitive) || fileContent.contains("from anthropic", Qt::CaseInsensitive))
        return ProgrammingLanguage::Python;
    if (fileContent.contains("@anthropic-ai/sdk", Qt::CaseInsensitive) || fileContent.contains("import Anthropic from", Qt::CaseInsensitive))
        return ProgrammingLanguage::TypeScript;
    if (fileContent.contains("import com.anthropic", Qt::CaseInsensitive))
        return ProgrammingLanguage::Java;
    if (fileContent.contains("github.com/anthropics", Qt::CaseInsensitive))
        return ProgrammingLanguage::Go;

    return ProgrammingLanguage::Python;  // Default
}

QString SkillAPIReference::languageToString(ProgrammingLanguage lang) const
{
    switch (lang) {
    case ProgrammingLanguage::Python: return "Python";
    case ProgrammingLanguage::TypeScript: return "TypeScript";
    case ProgrammingLanguage::Java: return "Java";
    case ProgrammingLanguage::Go: return "Go";
    case ProgrammingLanguage::Ruby: return "Ruby";
    case ProgrammingLanguage::CSharp: return "C#";
    case ProgrammingLanguage::PHP: return "PHP";
    case ProgrammingLanguage::cURL: return "cURL";
    }
    return "Unknown";
}

SkillAPIReference::ProgrammingLanguage SkillAPIReference::stringToLanguage(
    const QString &langStr) const
{
    if (langStr.toLower() == "python") return ProgrammingLanguage::Python;
    if (langStr.toLower() == "typescript") return ProgrammingLanguage::TypeScript;
    if (langStr.toLower() == "javascript") return ProgrammingLanguage::TypeScript;
    if (langStr.toLower() == "java") return ProgrammingLanguage::Java;
    if (langStr.toLower() == "go") return ProgrammingLanguage::Go;
    if (langStr.toLower() == "ruby") return ProgrammingLanguage::Ruby;
    if (langStr.toLower() == "c#" || langStr.toLower() == "csharp") return ProgrammingLanguage::CSharp;
    if (langStr.toLower() == "php") return ProgrammingLanguage::PHP;
    if (langStr.toLower() == "curl") return ProgrammingLanguage::cURL;
    
    return ProgrammingLanguage::Python;  // Default
}
