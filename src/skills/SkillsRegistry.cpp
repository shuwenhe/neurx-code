#include "SkillsRegistry.h"
#include <QDebug>

SkillsRegistry::SkillsRegistry(QObject *parent)
    : QObject(parent)
{
    // Initialize all skills
    m_apiReference = std::make_shared<SkillAPIReference>();
    m_mcpBuilder = std::make_shared<SkillMCPBuilder>();
    m_skillCreator = std::make_shared<SkillCreator>();
    m_resourceManager = std::make_shared<SkillResourceManager>();
}

void SkillsRegistry::initialize(const QString &resourcesPath)
{
    m_resourcesPath = resourcesPath;
    qDebug() << "Skills Registry initialized with resources at:" << resourcesPath;
}

QVector<QString> SkillsRegistry::getAvailableSkills() const
{
    return {
        "claude-api",
        "mcp-builder",
        "skill-creator",
        "web-testing"
    };
}

QString SkillsRegistry::getSkillDescription(const QString &skillId) const
{
    if (skillId == "claude-api") {
        return "Reference for Claude API and SDKs - model info, pricing, migration guides, "
               "error handling, tool use, streaming, caching, batch processing";
    }
    else if (skillId == "mcp-builder") {
        return "Guide for building high-quality MCP servers with comprehensive tool coverage, "
               "authentication patterns, best practices, performance optimization, security guidelines";
    }
    else if (skillId == "skill-creator") {
        return "Framework for creating, evaluating, benchmarking, and optimizing Claude skills "
               "with test case generation, variance analysis, trigger optimization";
    }
    else if (skillId == "web-testing") {
        return "Playwright-based web testing automation with server lifecycle management, "
               "visual regression testing, accessibility testing";
    }
    return "Unknown skill";
}

QVector<QString> SkillsRegistry::getSkillCapabilities(const QString &skillId) const
{
    if (skillId == "claude-api") {
        return {
            "model_information",
            "sdk_reference",
            "pricing_info",
            "api_patterns",
            "tool_use",
            "streaming",
            "vision",
            "caching",
            "batch_processing",
            "migration_guides"
        };
    }
    else if (skillId == "mcp-builder") {
        return {
            "project_generation",
            "tool_definition",
            "authentication_setup",
            "error_handling",
            "testing_framework",
            "documentation_generation",
            "performance_patterns",
            "security_guidelines",
            "scalability_patterns"
        };
    }
    else if (skillId == "skill-creator") {
        return {
            "skill_templates",
            "skill_validation",
            "evaluation_generation",
            "test_execution",
            "performance_analysis",
            "trigger_optimization",
            "variance_analysis",
            "benchmarking",
            "improvement_suggestions"
        };
    }
    else if (skillId == "web-testing") {
        return {
            "browser_automation",
            "visual_testing",
            "accessibility_testing",
            "performance_monitoring",
            "screenshot_capture"
        };
    }
    return {};
}

QVector<QString> SkillsRegistry::searchSkills(const QString &query, int maxResults) const
{
    QVector<QString> results;
    QString queryLower = query.toLower();

    auto allSkills = getAvailableSkills();
    
    for (const auto &skillId : allSkills) {
        auto desc = getSkillDescription(skillId);
        auto caps = getSkillCapabilities(skillId);
        
        // Search in skill ID, description, and capabilities
        if (skillId.contains(queryLower) || 
            desc.toLower().contains(queryLower)) {
            results.append(skillId);
            if (results.count() >= maxResults) {
                return results;
            }
        }

        for (const auto &cap : caps) {
            if (cap.contains(queryLower)) {
                if (!results.contains(skillId)) {
                    results.append(skillId);
                }
                if (results.count() >= maxResults) {
                    return results;
                }
            }
        }
    }

    return results;
}

QString SkillsRegistry::getSkillContextForLLM(const QString &skillId, int tier) const
{
    QString context;

    if (skillId == "claude-api") {
        if (tier == 1) {
            // Lightweight tier
            context = "claude-api: Reference for Claude API, models, SDKs, pricing, patterns";
        }
        else if (tier == 2) {
            // Full metadata tier
            auto models = m_apiReference->getAvailableModels();
            context = "claude-api: Provides comprehensive reference for:\n";
            context += "- Available models: ";
            for (int i = 0; i < std::min(3, static_cast<int>(models.count())); ++i) {
                context += models[i].modelId + " ";
            }
            context += "\n- SDK references for Python, TypeScript, Java, Go, Ruby, C#, PHP\n";
            context += "- API patterns: streaming, tool-use, vision, caching, batch processing\n";
            context += "- Pricing, rate limiting, error handling, best practices";
        }
    }
    else if (skillId == "mcp-builder") {
        if (tier == 1) {
            context = "mcp-builder: Guide for building MCP servers for LLM integration";
        }
        else if (tier == 2) {
            context = "mcp-builder: Comprehensive MCP server development guide including:\n";
            context += "- Project scaffolding for TypeScript and Python\n";
            context += "- Tool definition and implementation templates\n";
            context += "- Authentication patterns (bearer, OAuth, API key)\n";
            context += "- Error handling, validation, logging frameworks\n";
            context += "- Testing and mock server generation\n";
            context += "- Best practices for performance, security, scalability\n";
            context += "- Common patterns: pagination, retry logic, rate limiting, caching";
        }
    }
    else if (skillId == "skill-creator") {
        if (tier == 1) {
            context = "skill-creator: Framework for creating and optimizing Claude skills";
        }
        else if (tier == 2) {
            context = "skill-creator: Complete skill development framework:\n";
            context += "- Skill templates for different categories\n";
            context += "- Interview-guided skill creation\n";
            context += "- Evaluation case generation and execution\n";
            context += "- Performance analysis and benchmarking\n";
            context += "- Trigger optimization for better activation\n";
            context += "- Variance analysis and statistical evaluation\n";
            context += "- Iterative improvement suggestions";
        }
    }
    else if (skillId == "web-testing") {
        if (tier == 1) {
            context = "web-testing: Playwright-based web automation and testing";
        }
        else if (tier == 2) {
            context = "web-testing: Web testing and automation framework:\n";
            context += "- Browser automation (Chrome, Firefox, Safari)\n";
            context += "- Visual regression testing\n";
            context += "- Accessibility testing\n";
            context += "- Performance monitoring\n";
            context += "- Screenshot and recording capabilities";
        }
    }

    if (context.isEmpty()) {
        context = QString("Skill '%1' not found").arg(skillId);
    }

    return context;
}

QMap<QString, int> SkillsRegistry::getSkillStats() const
{
    QMap<QString, int> stats;
    stats["total_skills"] = getTotalSkillsLoaded();
    stats["api_reference_models"] = m_apiReference->getAvailableModels().count();
    stats["mcp_patterns"] = m_mcpBuilder->getBestPractices().count();
    stats["skill_templates"] = m_skillCreator->getSkillTemplates().count();
    stats["resource_cache"] = m_resourceManager->getCacheStats().count();
    return stats;
}
