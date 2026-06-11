# Claude Code Skills Quick Reference

## Quick Start

### 1. Initialize Skills Registry

```cpp
#include "src/skills/SkillsRegistry.h"

auto registry = std::make_shared<SkillsRegistry>();
registry->initialize(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/skills");
```

### 2. List Available Skills

```cpp
auto skills = registry->getAvailableSkills();
// Returns: ["claude-api", "mcp-builder", "skill-creator"]

for (const auto &skillId : skills) {
    qDebug() << skillId << ":" << registry->getSkillDescription(skillId);
}
```

### 3. Search Skills

```cpp
auto results = registry->searchSkills("streaming", 5);
// Returns skills matching "streaming"
```

### 4. Get Skill Context for LLM

```cpp
QString context = registry->getSkillContextForLLM("claude-api", 2);
// Tier 1: Lightweight (~50 tokens)
// Tier 2: Full metadata (~200 tokens)
// Tier 3: Complete content (unlimited)
```

---

## Skill-Specific Quick Reference

### SkillAPIReference

**Get Model Information**
```cpp
auto apiSkill = registry->getAPIReferenceSkill();
auto models = apiSkill->getAvailableModels();
for (const auto &model : models) {
    qDebug() << model.displayName << model.maxTokens << "tokens";
}
```

**Get SDK for Specific Language**
```cpp
auto lang = SkillAPIReference::ProgrammingLanguage::Python;
auto sdk = apiSkill->getSDKReference(lang);
qDebug() << "Install:" << sdk.installCommand;
```

**Auto-Detect Language**
```cpp
auto lang = apiSkill->detectLanguage(codeContent, "main.py");
auto guide = apiSkill->getStreamingGuide(lang);
```

**Get API Patterns**
```cpp
auto patterns = apiSkill->getAPIPatterns("streaming");
for (const auto &pattern : patterns) {
    qDebug() << pattern.name << "-" << pattern.codeExample;
}
```

---

### SkillMCPBuilder

**Generate MCP Project Structure**
```cpp
auto mcpBuilder = registry->getMCPBuilder();

SkillMCPBuilder::MCPServerProject project;
project.name = "my-api-server";
project.description = "Integration with external API";
project.language = SkillMCPBuilder::Language::TypeScript;
project.apiBaseUrl = "https://api.example.com";

QString structure = mcpBuilder->generateProjectStructure(project, "/tmp/output");
```

**Define and Generate Tools**
```cpp
SkillMCPBuilder::ToolDefinition tool;
tool.name = "create_resource";
tool.description = "Create new resource in the API";

QString implementation = mcpBuilder->generateToolImplementation(
    tool, 
    SkillMCPBuilder::Language::TypeScript
);
```

**Generate Authentication Module**
```cpp
QString authCode = mcpBuilder->generateAuthModule("bearer", SkillMCPBuilder::Language::Python);
```

**Get Best Practices**
```cpp
auto practices = mcpBuilder->getBestPractices("security");
for (const auto &practice : practices) {
    qDebug() << practice.name << ":" << practice.guidance;
}
```

**Generate Common Patterns**
```cpp
// Pagination
QString pagination = mcpBuilder->generatePaginationPattern(SkillMCPBuilder::Language::TypeScript);

// Retry logic
QString retry = mcpBuilder->generateRetryLogic(SkillMCPBuilder::Language::Python);

// Rate limiting
QString rateLim = mcpBuilder->generateRateLimiting(SkillMCPBuilder::Language::TypeScript);

// Caching
QString cache = mcpBuilder->generateCaching(SkillMCPBuilder::Language::Python);
```

---

### SkillCreator

**Create Skill from Template**
```cpp
auto skillCreator = registry->getSkillCreator();

QMap<QString, QString> vars;
vars["SKILL_NAME"] = "DocumentProcessor";
vars["SKILL_DESCRIPTION"] = "Process and extract data from documents";
vars["PURPOSE"] = "Extract structured data";

QString skillContent = skillCreator->createSkillFromTemplate(
    "DocumentProcessor",
    "BasicSkill",
    vars
);
```

**Interview-Guided Skill Creation**
```cpp
auto questions = skillCreator->getInterviewQuestions();
QMap<QString, QString> answers;
// User fills in answers to questions

QString skill = skillCreator->generateSkillFromAnswers(answers);
```

**Generate and Run Evaluations**
```cpp
auto testCases = skillCreator->generateEvaluationCases(
    "my-skill",
    "Skill description",
    10  // number of test cases
);

auto performance = skillCreator->runAllEvaluations(
    "my-skill",
    [](const QString &prompt) {
        // Execute skill with prompt
        return "result";
    }
);

qDebug() << skillCreator->analyzePerformance(performance);
```

**Get Optimization Recommendations**
```cpp
auto recommendations = skillCreator->getOptimizationRecommendations(performance);
for (const auto &rec : recommendations) {
    qDebug() << rec;
}
```

**Optimize Skill Description**
```cpp
auto optimized = skillCreator->optimizeSkillDescription(
    currentDescription,
    testCases,
    performance.results
);

qDebug() << "Optimized trigger score:" << optimized.triggerScore;
```

**Benchmark and Compare**
```cpp
QString variance = skillCreator->performVarianceAnalysis(performance.results);
qDebug() << variance;

QString comparison = skillCreator->compareSkillVersions(perfV1, perfV2);
qDebug() << comparison;
```

---

### SkillResourceManager

**Load Resources**
```cpp
auto resourceMgr = registry->getResourceManager();

auto scriptRes = resourceMgr->loadResource(
    "skill-id",
    "evaluation.py",
    SkillResourceManager::ResourceType::Script
);

qDebug() << "Script loaded:" << scriptRes.content.length() << "bytes";
```

**Execute Script**
```cpp
SkillResourceManager::ScriptExecution exec;
exec.skillId = "my-skill";
exec.scriptName = "test.py";
exec.arguments << "arg1" << "arg2";
exec.timeoutMs = 5000;

auto result = resourceMgr->executeScript(exec);
qDebug() << "Exit code:" << result.exitCode;
qDebug() << "Output:" << result.stdout;
if (!result.success) {
    qDebug() << "Error:" << result.error;
}
```

**Process Templates**
```cpp
QString templateContent = "API Key: {{API_KEY}}\nBase URL: {{BASE_URL}}";
QMap<QString, QString> vars;
vars["API_KEY"] = "sk_test_123";
vars["BASE_URL"] = "https://api.example.com";

QString processed = resourceMgr->processTemplate(templateContent, vars);
```

**Load and Process Template**
```cpp
QString result = resourceMgr->loadAndProcessTemplate(
    "skill-id",
    "config.template",
    vars
);
```

**Cache Management**
```cpp
auto stats = resourceMgr->getCacheStats();
for (auto it = stats.begin(); it != stats.end(); ++it) {
    qDebug() << it.key() << ":" << it.value() << "resources cached";
}

resourceMgr->clearCache("skill-id");  // Clear specific skill
resourceMgr->clearAllCaches();         // Clear all caches
```

---

## Common Tasks

### Task 1: Generate API Client Code

```cpp
auto apiSkill = registry->getAPIReferenceSkill();

// Detect language
auto lang = apiSkill->detectLanguage(userCode, "main.ts");

// Get quick start
QString startCode = apiSkill->getQuickStart(lang);

// Get streaming pattern if needed
QString streamCode = apiSkill->getStreamingGuide(lang);

// Get error handling
QString errorCode = apiSkill->getErrorHandlingGuide(lang);
```

### Task 2: Create MCP Server

```cpp
auto mcpBuilder = registry->getMCPBuilder();

// Define project
SkillMCPBuilder::MCPServerProject project;
project.name = "slack-api";
project.language = SkillMCPBuilder::Language::TypeScript;
project.apiBaseUrl = "https://slack.com/api";
project.apiAuthType = "bearer";

// Generate structure
mcpBuilder->generateProjectStructure(project, outputPath);

// Generate files
QString packageJson = mcpBuilder->generatePackageJson(project);
QString authModule = mcpBuilder->generateAuthModule("bearer", project.language);
QString apiClient = mcpBuilder->generateAPIClient(project, project.language);
```

### Task 3: Create and Test Skill

```cpp
auto skillCreator = registry->getSkillCreator();
auto resourceMgr = registry->getResourceManager();

// Create from template
QString skillContent = skillCreator->createSkillFromTemplate(
    "ImageAnalyzer", "BasicSkill", vars);

// Generate test cases
auto testCases = skillCreator->generateEvaluationCases("image-analyzer", "", 5);

// Add edge cases
auto edgeCases = skillCreator->generateEdgeCasesTests("image analysis", 3);
testCases.append(edgeCases);

// Run evaluations
auto performance = skillCreator->runAllEvaluations("image-analyzer", executeFunc);

// Analyze results
QString analysis = skillCreator->analyzePerformance(performance);
auto recommendations = skillCreator->getOptimizationRecommendations(performance);

// Optimize
auto optimized = skillCreator->optimizeSkillDescription(skillContent, testCases, performance.results);

// Generate report
QString report = skillCreator->generateEvaluationReport(performance);
```

---

## Tips & Tricks

### Tip 1: Lazy Loading
Only initialize skills when needed:
```cpp
SkillAPIReferencePtr apiSkill;
if (needsAPIReference) {
    apiSkill = registry->getAPIReferenceSkill();
}
```

### Tip 2: Cache Frequently Used Resources
```cpp
auto resourceMgr = registry->getResourceManager();
// First load caches the resource
auto res1 = resourceMgr->loadResource("skill", "template.html", ResourceType::Template);
// Subsequent loads are instant
auto res2 = resourceMgr->loadResource("skill", "template.html", ResourceType::Template);
```

### Tip 3: Batch Operations
```cpp
// Get all skills context at once
QString skillsContext;
for (const auto &skillId : registry->getAvailableSkills()) {
    skillsContext += registry->getSkillContextForLLM(skillId, 1) + "\n";
}
```

### Tip 4: Error Handling
```cpp
auto apiSkill = registry->getAPIReferenceSkill();
auto model = apiSkill->getModelInfo("claude-3-nonexistent");
if (model.modelId.isEmpty()) {
    qDebug() << "Model not found";
}
```

### Tip 5: Template Variables
Always use uppercase with underscores:
```cpp
vars["API_KEY"] = "...";      // ✓ Good
vars["apiKey"] = "...";        // ✗ Won't replace {{API_KEY}}
```

---

## Performance Considerations

| Operation | Time | Notes |
|-----------|------|-------|
| getAvailableModels() | <1ms | Cached in memory |
| detectLanguage() | <1ms | Pattern matching |
| generateProjectStructure() | ~10ms | File template processing |
| loadResource() | ~5ms | First load; <1ms cached |
| executeScript() | 100ms-5s | Depends on script |

---

## API Reference

### Resource Types
```cpp
enum class ResourceType {
    Script,      // .py, .sh, .js
    Template,    // HTML, code templates
    Reference,   // Markdown docs
    Asset        // Images, fonts, data
};
```

### Languages
```cpp
enum class ProgrammingLanguage {
    Python, TypeScript, Java, Go, Ruby, CSharp, PHP, cURL
};
```

### Skill Categories
```
- technical      (API, tools, frameworks)
- enterprise     (workflows, communications)
- creative       (design, art, content)
- document       (file generation)
```

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Skills not found | Check resource path and initialize() called |
| Template vars not replaced | Verify {{UPPERCASE}} format in template |
| Script fails | Check script exists, interpreter available, has execute permission |
| Slow performance | Check cache stats, clear old cache entries |
| Memory usage high | Call clearCache() or clearAllCaches() periodically |

---

## Links

- **Full Guide**: See CLAUDE_CODE_SKILLS_INTEGRATION.md
- **Header Files**: `/Users/feifei/agent/neurx-code/src/skills/`
- **Implementation**: Same directory as headers
- **Original Skills**: `/Users/feifei/agent/skills/`
