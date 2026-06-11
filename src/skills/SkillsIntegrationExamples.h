#pragma once

#include "SkillsRegistry.h"
#include <QString>
#include <QMap>

/**
 * @class SkillsIntegrationExample
 * @brief Example implementations showing how to use Claude Code Skills in neurx-code
 * 
 * This file demonstrates practical usage patterns for:
 * - Integrating skills with agent tools
 * - Using skills in code generation pipelines
 * - Implementing skill-based workflows
 */

// ────────────────────────────────────────────────────────────────────────────
// Example 1: API Code Generator Tool
// ────────────────────────────────────────────────────────────────────────────

class APICodeGeneratorExample {
public:
    static QString generateClientCode(
        const QString &userDescription,
        const QString &targetLanguage,
        SkillsRegistryPtr registry)
    {
        auto apiSkill = registry->getAPIReferenceSkill();
        
        // Step 1: Detect or convert language
        auto lang = apiSkill->stringToLanguage(targetLanguage);
        
        // Step 2: Get quick start code
        QString code = apiSkill->getQuickStart(lang);
        
        // Step 3: Get appropriate pattern based on description
        QString patternName;
        if (userDescription.toLower().contains("stream")) {
            patternName = "Streaming";
        } else if (userDescription.toLower().contains("image")) {
            patternName = "Vision";
        } else if (userDescription.toLower().contains("cache")) {
            patternName = "Prompt Caching";
        } else {
            patternName = "Basic Message";
        }
        
        QString pattern = apiSkill->getPatternExample(patternName, lang);
        
        // Step 4: Add error handling guide
        QString errorHandling = apiSkill->getErrorHandlingGuide(lang);
        
        // Step 5: Combine everything
        return QString(
            "// Generated Claude API Client Code\n"
            "// Language: %1\n"
            "// Description: %2\n\n"
            "// Basic Setup\n%3\n\n"
            "// Pattern: %4\n%5\n\n"
            "// Error Handling\n%6"
        ).arg(targetLanguage, userDescription, code, patternName, pattern, errorHandling);
    }
};

// ────────────────────────────────────────────────────────────────────────────
// Example 2: MCP Server Generator Tool
// ────────────────────────────────────────────────────────────────────────────

class MCPServerGeneratorExample {
public:
    struct ServerRequest {
        QString name;
        QString description;
        QString apiBaseUrl;
        QString authType;  // "bearer", "api_key", "oauth", "none"
        QVector<QString> requiredTools;
        QString preferredLanguage;  // "TypeScript" or "Python"
    };

    static QString generateMCPServer(
        const ServerRequest &request,
        SkillsRegistryPtr registry)
    {
        auto mcpBuilder = registry->getMCPBuilder();
        
        // Step 1: Determine language
        auto language = (request.preferredLanguage == "Python") 
            ? SkillMCPBuilder::Language::Python 
            : SkillMCPBuilder::Language::TypeScript;
        
        // Step 2: Create project
        SkillMCPBuilder::MCPServerProject project;
        project.name = request.name;
        project.description = request.description;
        project.language = language;
        project.apiBaseUrl = request.apiBaseUrl;
        project.apiAuthType = request.authType;
        project.version = "1.0.0";
        
        // Step 3: Add recommended tools for service
        auto serviceType = mcpBuilder->detectServiceType(request.description);
        auto toolNames = mcpBuilder->generateToolNamesForService(
            serviceType,
            request.requiredTools.isEmpty() ? 5 : request.requiredTools.count()
        );
        
        for (const auto &toolName : toolNames) {
            SkillMCPBuilder::ToolDefinition tool;
            tool.name = toolName;
            tool.description = QString("Tool: %1").arg(toolName);
            project.tools.append(tool);
        }
        
        // Step 4: Generate all artifacts
        QString result = "# MCP Server Generation Results\n\n";
        
        result += "## Project Structure\n```\n" + 
                  mcpBuilder->generateProjectStructure(project, "/tmp/output") + 
                  "\n```\n\n";
        
        if (language == SkillMCPBuilder::Language::TypeScript) {
            result += "## package.json\n```json\n" + 
                      mcpBuilder->generatePackageJson(project) + 
                      "\n```\n\n";
        } else {
            result += "## pyproject.toml\n```toml\n" + 
                      mcpBuilder->generatePyprojectToml(project) + 
                      "\n```\n\n";
        }
        
        result += "## Authentication Module\n```" + 
                  (language == SkillMCPBuilder::Language::Python ? "python" : "typescript") + 
                  "\n" +
                  mcpBuilder->generateAuthModule(request.authType, language) + 
                  "\n```\n\n";
        
        result += "## README\n\n" + mcpBuilder->generateREADME(project);
        
        return result;
    }
};

// ────────────────────────────────────────────────────────────────────────────
// Example 3: Skill Creation Workflow
// ────────────────────────────────────────────────────────────────────────────

class SkillCreationWorkflowExample {
public:
    static QString createAndTestSkill(
        const QString &skillName,
        const QString &skillDescription,
        SkillsRegistryPtr registry,
        std::function<QString(const QString &)> executeFunc)
    {
        auto skillCreator = registry->getSkillCreator();
        
        QString result;
        
        // Phase 1: Create Skill
        result += "# Skill Creation Workflow: " + skillName + "\n\n";
        result += "## Phase 1: Skill Creation\n";
        
        QMap<QString, QString> vars;
        vars["SKILL_NAME"] = skillName;
        vars["SKILL_DESCRIPTION"] = skillDescription;
        vars["PURPOSE"] = skillDescription;
        vars["WHEN_TO_USE"] = "the user requests " + skillName.toLower();
        vars["LICENSE"] = "MIT";
        
        QString skillContent = skillCreator->createSkillFromTemplate(
            skillName, "BasicSkill", vars);
        
        result += "✓ Skill created from template\n";
        
        // Phase 2: Validation
        result += "\n## Phase 2: Validation\n";
        QString validationError;
        if (skillCreator->validateSkill(skillContent, validationError)) {
            result += "✓ Skill validation passed\n";
        } else {
            result += "✗ Validation failed: " + validationError + "\n";
            return result;
        }
        
        // Phase 3: Evaluation Cases
        result += "\n## Phase 3: Test Generation\n";
        auto testCases = skillCreator->generateEvaluationCases(
            skillName.toLower(),
            skillDescription,
            10
        );
        result += QString("✓ Generated %1 test cases\n").arg(testCases.count());
        
        // Add edge cases
        auto edgeCases = skillCreator->generateEdgeCasesTests(skillDescription, 3);
        testCases.append(edgeCases);
        result += QString("✓ Added %1 edge case tests\n").arg(edgeCases.count());
        
        // Phase 4: Evaluation
        result += "\n## Phase 4: Evaluation & Analysis\n";
        
        auto performance = skillCreator->runAllEvaluations(
            skillName.toLower(),
            executeFunc
        );
        
        result += QString("✓ Ran %1 tests\n").arg(performance.totalTests);
        result += QString("✓ Passed: %1/%2 (%.1f%%)\n")
            .arg(performance.passedTests)
            .arg(performance.totalTests)
            .arg(performance.passRate);
        result += QString("✓ Average Score: %.1f\n").arg(performance.averageScore);
        result += QString("✓ Avg Response Time: %.0f ms\n")
            .arg(performance.averageResponseTimeMs);
        
        // Phase 5: Recommendations
        result += "\n## Phase 5: Recommendations\n";
        auto recommendations = skillCreator->getOptimizationRecommendations(performance);
        if (recommendations.isEmpty()) {
            result += "✓ No optimization recommendations\n";
        } else {
            for (const auto &rec : recommendations) {
                result += "- " + rec + "\n";
            }
        }
        
        // Phase 6: Optimization
        result += "\n## Phase 6: Optimization\n";
        auto optimized = skillCreator->optimizeSkillDescription(
            skillContent, testCases, performance.results);
        result += QString("✓ Optimized description\n");
        result += QString("✓ Trigger effectiveness: %.1f%%\n").arg(optimized.triggerScore);
        
        // Phase 7: Report
        result += "\n## Phase 7: Final Report\n";
        result += skillCreator->generateEvaluationReport(performance);
        
        return result;
    }
};

// ────────────────────────────────────────────────────────────────────────────
// Example 4: Agent Context Enhancement
// ────────────────────────────────────────────────────────────────────────────

class AgentContextEnhancerExample {
public:
    static QString buildSkillContextForAgent(SkillsRegistryPtr registry)
    {
        QString context = "## Available Skills\n\n";
        
        for (const auto &skillId : registry->getAvailableSkills()) {
            // Get Tier 1 (lightweight) context
            QString skillContext = registry->getSkillContextForLLM(skillId, 1);
            
            QString capabilities;
            auto caps = registry->getSkillCapabilities(skillId);
            for (int i = 0; i < std::min(3, static_cast<int>(caps.count())); ++i) {
                if (i > 0) capabilities += ", ";
                capabilities += caps[i];
            }
            if (caps.count() > 3) capabilities += ", ...";
            
            context += QString(
                "### %1\n"
                "%2\n"
                "**Capabilities:** %3\n\n"
            ).arg(skillId, skillContext, capabilities);
        }
        
        return context;
    }

    static QString buildDetailedSkillContext(
        const QString &skillId,
        SkillsRegistryPtr registry)
    {
        // Get Tier 2 (full metadata) context
        return registry->getSkillContextForLLM(skillId, 2);
    }
};

// ────────────────────────────────────────────────────────────────────────────
// Example 5: Resource Management
// ────────────────────────────────────────────────────────────────────────────

class ResourceManagementExample {
public:
    static QString executeSkillScript(
        const QString &skillId,
        const QString &scriptName,
        const QStringList &arguments,
        SkillsRegistryPtr registry)
    {
        auto resourceMgr = registry->getResourceManager();
        
        // Execute script
        SkillResourceManager::ScriptExecution exec;
        exec.skillId = skillId;
        exec.scriptName = scriptName;
        exec.arguments = arguments;
        exec.timeoutMs = 30000;  // 30 seconds
        
        auto result = resourceMgr->executeScript(exec);
        
        if (!result.success) {
            return QString(
                "Error executing script: %1\n"
                "Exit code: %2"
            ).arg(result.error).arg(result.exitCode);
        }
        
        return QString(
            "Script executed successfully\n"
            "Output:\n%1\n\n"
            "Execution time: %2 ms"
        ).arg(result.stdout).arg(result.executionTimeMs);
    }

    static QString processAndLoadTemplate(
        const QString &skillId,
        const QString &templateName,
        const QMap<QString, QString> &variables,
        SkillsRegistryPtr registry)
    {
        auto resourceMgr = registry->getResourceManager();
        
        // Load and process template
        QString processed = resourceMgr->loadAndProcessTemplate(
            skillId,
            templateName,
            variables
        );
        
        return processed;
    }

    static void printCacheStats(SkillsRegistryPtr registry)
    {
        auto stats = registry->getSkillStats();
        
        qDebug() << "Skill Statistics:";
        for (auto it = stats.begin(); it != stats.end(); ++it) {
            qDebug() << "  " << it.key() << ":" << it.value();
        }
    }
};

// ────────────────────────────────────────────────────────────────────────────
// Example 6: Complete Workflow Pipeline
// ────────────────────────────────────────────────────────────────────────────

class CompleteWorkflowExample {
public:
    static QString executeFullPipeline(
        const QString &userRequest,
        SkillsRegistryPtr registry)
    {
        QString output = "## Full Workflow Pipeline\n\n";
        
        // Step 1: Search for relevant skills
        output += "### Step 1: Skill Discovery\n";
        auto relevantSkills = registry->searchSkills(userRequest, 3);
        output += QString("Found %1 relevant skills:\n").arg(relevantSkills.count());
        for (const auto &skill : relevantSkills) {
            output += "- " + skill + "\n";
        }
        output += "\n";
        
        // Step 2: Get context for each relevant skill
        output += "### Step 2: Context Gathering\n";
        for (const auto &skillId : relevantSkills) {
            auto context = registry->getSkillContextForLLM(skillId, 2);
            output += QString("**%1:**\n%2\n\n").arg(skillId, context);
        }
        
        // Step 3: Use appropriate skill based on request
        output += "### Step 3: Processing\n";
        
        if (userRequest.toLower().contains("api")) {
            output += "Using SkillAPIReference...\n";
            auto apiSkill = registry->getAPIReferenceSkill();
            auto models = apiSkill->getAvailableModels();
            output += QString("Found %1 available models\n").arg(models.count());
        }
        else if (userRequest.toLower().contains("mcp")) {
            output += "Using SkillMCPBuilder...\n";
            auto mcpBuilder = registry->getMCPBuilder();
            auto practices = mcpBuilder->getBestPractices();
            output += QString("Found %1 best practices\n").arg(practices.count());
        }
        else if (userRequest.toLower().contains("skill")) {
            output += "Using SkillCreator...\n";
            auto skillCreator = registry->getSkillCreator();
            auto templates = skillCreator->getSkillTemplates();
            output += QString("Found %1 templates\n").arg(templates.count());
        }
        
        return output;
    }
};
