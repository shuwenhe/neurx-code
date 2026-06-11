# Claude Code Skills Implementation Guide for neurx-code

## Overview

This guide describes the implementation of Claude Code Skills from `/Users/feifei/agent/skills` into the neurx-code application. Four major skills have been fully implemented:

1. **SkillAPIReference** - Claude API and SDK documentation
2. **SkillMCPBuilder** - MCP server development framework  
3. **SkillCreator** - Skill creation and evaluation tools
4. **SkillResourceManager** - Resource loading and script execution

## Architecture

### Component Hierarchy

```
SkillsRegistry (Central Hub)
├── SkillAPIReference
│   ├── Models Information
│   ├── SDK References
│   ├── API Patterns
│   ├── Streaming, Vision, Caching
│   ├── Migration Guides
│   └── Best Practices
├── SkillMCPBuilder
│   ├── Project Generation
│   ├── Tool Definition
│   ├── Authentication Setup
│   ├── Error Handling
│   ├── Testing Framework
│   ├── Documentation Generation
│   └── Common Patterns
├── SkillCreator
│   ├── Skill Templates
│   ├── Interview Framework
│   ├── Evaluation System
│   ├── Performance Analysis
│   ├── Trigger Optimization
│   └── Benchmarking
└── SkillResourceManager
    ├── Script Execution
    ├── Template Processing
    ├── Resource Loading
    └── Caching
```

## Key Classes and Interfaces

### 1. SkillAPIReference
**Purpose:** Provides comprehensive Claude API documentation and SDK references

**Key Methods:**
```cpp
// Model Information
QVector<ModelInfo> getAvailableModels() const;
QString getPricingInfo(const QString &modelId) const;
QString getLatestRecommendedModel() const;

// SDK Reference
SDKReference getSDKReference(ProgrammingLanguage language) const;
QString getQuickStart(ProgrammingLanguage language) const;

// API Patterns
QVector<APIPattern> getAPIPatterns(const QString &category) const;
QString getPatternExample(const QString &patternName, ProgrammingLanguage language) const;

// Advanced Features
QString getStreamingGuide(ProgrammingLanguage language) const;
QString getVisionGuide(ProgrammingLanguage language) const;
QString getPromptCachingGuide(ProgrammingLanguage language) const;

// Language Detection
ProgrammingLanguage detectLanguage(const QString &fileContent, const QString &fileName) const;
```

**Usage Example:**
```cpp
auto apiSkill = registry->getAPIReferenceSkill();

// Get model info
auto models = apiSkill->getAvailableModels();
for (const auto &model : models) {
    qDebug() << model.displayName << ":" << model.pricePerMTok;
}

// Get SDK for detected language
auto lang = apiSkill->detectLanguage(codeContent, "main.ts");
auto sdk = apiSkill->getSDKReference(lang);
qDebug() << "Install:" << sdk.installCommand;

// Get streaming guide
qDebug() << apiSkill->getStreamingGuide(ProgrammingLanguage::Python);
```

### 2. SkillMCPBuilder
**Purpose:** Framework for building MCP servers with best practices

**Key Methods:**
```cpp
// Project Generation
QString generateProjectStructure(const MCPServerProject &project, const QString &outputPath);
QString generatePackageJson(const MCPServerProject &project) const;
QString generateServerMain(const MCPServerProject &project) const;

// Tool Management
QString generateToolDefinition(const ToolDefinition &tool) const;
QString generateToolImplementation(const ToolDefinition &tool, Language language) const;
QString generateToolRegistry(const QVector<ToolDefinition> &tools, Language language) const;

// Authentication
QString generateAuthModule(const QString &authType, Language language) const;

// Common Patterns
QString generatePaginationPattern(Language language) const;
QString generateRetryLogic(Language language) const;
QString generateRateLimiting(Language language) const;
QString generateCaching(Language language) const;

// Documentation
QString generateREADME(const MCPServerProject &project) const;
QString generateAPIDocumentation(const MCPServerProject &project) const;
```

**Usage Example:**
```cpp
auto mcpBuilder = registry->getMCPBuilder();

// Define tools
SkillMCPBuilder::ToolDefinition tool;
tool.name = "get_user";
tool.description = "Fetch user information";

// Generate implementation
QString impl = mcpBuilder->generateToolImplementation(tool, Language::TypeScript);
qDebug() << impl;

// Generate best practices
auto practices = mcpBuilder->getBestPractices("design");
for (const auto &practice : practices) {
    qDebug() << practice.name << ":" << practice.guidance;
}
```

### 3. SkillCreator
**Purpose:** Framework for creating, evaluating, and optimizing skills

**Key Methods:**
```cpp
// Skill Creation
QVector<SkillTemplate> getSkillTemplates() const;
QString createSkillFromTemplate(const QString &skillName, const QString &templateName, 
                               const QMap<QString, QString> &variables);

// Interview-Guided Creation
QVector<QString> getInterviewQuestions() const;
QString generateSkillFromAnswers(const QMap<QString, QString> &answers);

// Evaluation
QVector<EvaluationCase> generateEvaluationCases(const QString &skillId, 
                                               const QString &skillDescription, int numCases);
SkillPerformance runAllEvaluations(const QString &skillId, 
                                  std::function<QString(const QString &)> executeSkill);

// Analysis
QString analyzePerformance(const SkillPerformance &performance) const;
QVector<QString> getOptimizationRecommendations(const SkillPerformance &performance) const;

// Optimization
SkillDescription optimizeSkillDescription(const QString &currentDescription, 
                                         const QVector<EvaluationCase> &testCases,
                                         const QVector<EvaluationResult> &results);

// Benchmarking
QString performVarianceAnalysis(const QVector<EvaluationResult> &results) const;
QString getBenchmarkStats(const QVector<SkillPerformance> &performances) const;
```

**Usage Example:**
```cpp
auto skillCreator = registry->getSkillCreator();

// Create skill from template
QMap<QString, QString> vars;
vars["SKILL_NAME"] = "MyCustomSkill";
vars["SKILL_DESCRIPTION"] = "Does amazing things";
QString skillContent = skillCreator->createSkillFromTemplate("MySkill", "BasicSkill", vars);

// Generate test cases
auto testCases = skillCreator->generateEvaluationCases("my-skill", "My skill description", 5);

// Run evaluations
auto performance = skillCreator->runAllEvaluations("my-skill", 
    [](const QString &prompt) { 
        // Execute skill logic
        return "result";
    }
);

// Analyze
qDebug() << skillCreator->analyzePerformance(performance);
auto recommendations = skillCreator->getOptimizationRecommendations(performance);
for (const auto &rec : recommendations) {
    qDebug() << rec;
}
```

### 4. SkillResourceManager
**Purpose:** Load, cache, and execute skill resources

**Key Methods:**
```cpp
// Resource Loading
SkillResource loadResource(const QString &skillId, const QString &resourceName, ResourceType type);
QVector<SkillResource> loadSkillResources(const QString &skillId);

// Script Execution
ScriptResult executeScript(const ScriptExecution &execution);
void executeScriptAsync(const ScriptExecution &execution, 
                       std::function<void(const ScriptResult &)> callback);

// Template Processing
QString processTemplate(const QString &template_content, 
                       const QMap<QString, QString> &variables);
QString loadAndProcessTemplate(const QString &skillId, const QString &templateName,
                              const QMap<QString, QString> &variables);

// Cache Management
void clearCache(const QString &skillId);
QMap<QString, int> getCacheStats() const;
```

**Usage Example:**
```cpp
auto resourceMgr = registry->getResourceManager();

// Load a script
auto script = resourceMgr->loadResource("skill-id", "evaluation.py", ResourceType::Script);

// Execute script
SkillResourceManager::ScriptExecution exec;
exec.skillId = "skill-id";
exec.scriptName = "evaluation.py";
exec.arguments << "test_param";
auto result = resourceMgr->executeScript(exec);
qDebug() << "Exit code:" << result.exitCode;
qDebug() << "Output:" << result.stdout;

// Process template
QMap<QString, QString> vars;
vars["API_KEY"] = "sk_test_123";
vars["BASE_URL"] = "https://api.example.com";
QString processed = resourceMgr->loadAndProcessTemplate("skill-id", "config.template", vars);
```

## Integration with neurx-code

### 1. Adding to CMakeLists.txt

```cmake
# Add skills sources to neurx_core library
target_sources(neurx_core PRIVATE
    src/skills/SkillResourceManager.cpp
    src/skills/SkillAPIReference.cpp
    src/skills/SkillMCPBuilder.cpp
    src/skills/SkillCreator.cpp
    src/skills/SkillsRegistry.cpp
)

# Link Qt modules (already required by other components)
target_link_libraries(neurx_core PUBLIC
    Qt6::Core
    Qt6::Concurrent
)
```

### 2. Initialization in Main Application

```cpp
#include "src/skills/SkillsRegistry.h"

// In your application initialization code
auto skillsRegistry = std::make_shared<SkillsRegistry>();

// Initialize with resource path
QString resourcePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/skills";
skillsRegistry->initialize(resourcePath);

// Get available skills
auto skills = skillsRegistry->getAvailableSkills();
qDebug() << "Available skills:" << skills;

// Use in agent/planning context
auto context = skillsRegistry->getSkillContextForLLM("claude-api", 2);
```

### 3. Integration with Agent System

```cpp
// In your agent's tool discovery or context generation
auto skillsRegistry = getSkillsRegistry();

// Enhance agent context with skill information
QString agentContext = "Available Skills:\n";
for (const auto &skillId : skillsRegistry->getAvailableSkills()) {
    agentContext += "- " + skillId + ": " + skillsRegistry->getSkillDescription(skillId) + "\n";
}

// Pass to LLM
QString systemPrompt = "You have access to Claude Code Skills:\n" + agentContext;
```

### 4. Using Skills in Tools

```cpp
// Tool implementation example: API code generation
class APIGenerationTool : public Tool {
    QString execute(const QJsonObject &input) override {
        auto skillsRegistry = getSkillsRegistry();
        auto apiSkill = skillsRegistry->getAPIReferenceSkill();
        
        // Detect language from input
        auto lang = apiSkill->detectLanguage(input["code"].toString());
        
        // Get SDK reference
        auto sdk = apiSkill->getSDKReference(lang);
        
        // Generate quick start code
        return apiSkill->getQuickStart(lang).toStdString();
    }
};

// Tool implementation example: MCP server generation
class MCPGeneratorTool : public Tool {
    QString execute(const QJsonObject &input) override {
        auto skillsRegistry = getSkillsRegistry();
        auto mcpBuilder = skillsRegistry->getMCPBuilder();
        
        // Define server
        SkillMCPBuilder::MCPServerProject project;
        project.name = input["name"].toString();
        project.description = input["description"].toString();
        project.language = SkillMCPBuilder::Language::TypeScript;
        
        // Generate structure
        return mcpBuilder->generateProjectStructure(project, "/tmp/output").toStdString();
    }
};
```

## Supported Skills Summary

| Skill | Purpose | Key Features |
|-------|---------|--------------|
| **claude-api** | SDK Reference & Documentation | 8+ models, 8 languages, pricing, patterns, migration guides |
| **mcp-builder** | MCP Server Development | Project scaffolding, tool generation, auth patterns, best practices |
| **skill-creator** | Skill Dev & Evaluation | Templates, interviews, evaluation framework, benchmarking, optimization |
| **resource-manager** | Resource Management | Script execution, template processing, caching, resource loading |

## Best Practices

1. **Lazy Loading**: Initialize skills only when needed
2. **Caching**: Leverage resource caching for frequently accessed data
3. **Language Detection**: Use `detectLanguage()` for automatic language identification
4. **Tier-Based Context**: Use tier 1 for listings, tier 2 for full metadata
5. **Error Handling**: Always check return values and error messages
6. **Template Variables**: Use consistent naming (all uppercase with underscores)
7. **Async Execution**: Use `executeScriptAsync()` for long-running scripts
8. **Resource Cleanup**: Call `clearCache()` when no longer needed

## Examples

### Example 1: Generate API Reference for Python

```cpp
auto apiSkill = registry->getAPIReferenceSkill();

// Get quick start for detected language
auto code = apiSkill->getQuickStart(SkillAPIReference::ProgrammingLanguage::Python);
qDebug() << "Python Quick Start:" << code;

// Get streaming guide
auto streaming = apiSkill->getStreamingGuide(SkillAPIReference::ProgrammingLanguage::Python);
qDebug() << streaming;

// Get best practices
auto practices = apiSkill->getBestPractices();
```

### Example 2: Create and Evaluate a Skill

```cpp
auto skillCreator = registry->getSkillCreator();

// Create from template
QMap<QString, QString> vars;
vars["PURPOSE"] = "Extract summary from documents";
vars["TRIGGERS"] = "extract, summary, document";
auto skillContent = skillCreator->createSkillFromTemplate("DocumentSummarizer", "BasicSkill", vars);

// Generate tests
auto tests = skillCreator->generateEvaluationCases("doc-summarizer", "Extract document summaries", 10);

// Run evaluation
auto perf = skillCreator->runAllEvaluations("doc-summarizer", 
    [](const QString &prompt) { /* execute skill */ return "summary"; });

// Analyze and optimize
auto analysis = skillCreator->analyzePerformance(perf);
auto optimized = skillCreator->optimizeSkillDescription(skillContent, tests, perf.results);
```

### Example 3: Generate MCP Server Code

```cpp
auto mcpBuilder = registry->getMCPBuilder();

// Define server
SkillMCPBuilder::MCPServerProject project;
project.name = "github-api";
project.description = "GitHub API integration server";
project.language = SkillMCPBuilder::Language::TypeScript;
project.apiBaseUrl = "https://api.github.com";

// Add tools
SkillMCPBuilder::ToolDefinition tool;
tool.name = "create_issue";
tool.description = "Create GitHub issue";
project.tools.append(tool);

// Generate all files
QString structure = mcpBuilder->generateProjectStructure(project, "/tmp/github-mcp");
QString packageJson = mcpBuilder->generatePackageJson(project);
QString readme = mcpBuilder->generateREADME(project);
```

## File Locations

- **Headers**: `/Users/feifei/agent/neurx-code/src/skills/`
- **Implementations**: `/Users/feifei/agent/neurx-code/src/skills/`
- **Resources**: `~/.local/share/neurx-code/skills/` (runtime)

## Future Enhancements

1. **Web Testing Skill**: Add Playwright-based web testing framework
2. **Document Skills**: Implement DOCX, PDF, XLSX, PPTX generation
3. **Creative Skills**: Add algorithmic art, design, theme generation
4. **Enterprise Skills**: Add communication templates, approval workflows
5. **Async Operations**: Full async/await support for all operations
6. **Caching**: Redis/SQLite backend for distributed caching
7. **Skill Marketplace**: Plugin system for third-party skills
8. **Telemetry**: Track skill usage and performance metrics

## Troubleshooting

### Skills not initializing
- Check resource path exists
- Verify CMakeLists.txt includes all skill sources
- Enable debug logging in SkillsRegistry

### Template variables not replacing
- Ensure variables use {{UPPERCASE_NAME}} format
- Check variable keys match exactly
- Verify map contains all required variables

### Script execution failing
- Check script file path is correct
- Verify script interpreter (python3, bash) is available
- Enable output capture to see error messages

## Support and Documentation

For more information:
- See individual skill class documentation in header files
- Review Claude Code Skills at `/Users/feifei/agent/skills`
- Check neurx-code integration examples above
