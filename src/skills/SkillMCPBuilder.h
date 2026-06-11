#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <QJsonObject>
#include <memory>

/**
 * @class SkillMCPBuilder
 * @brief MCP Server Development Skill
 * 
 * Provides guidance and tools for building high-quality Model Context Protocol (MCP) servers
 * that enable LLMs to interact with external services.
 * 
 * Supports:
 * - TypeScript/Node.js (recommended)
 * - Python (FastMCP)
 * - Best practices and design patterns
 * - Tool and resource definitions
 * - Transport mechanisms (stdio, HTTP)
 */
class SkillMCPBuilder {
public:
    enum class Language {
        TypeScript,
        Python
    };

    enum class TransportType {
        Stdio,
        HTTP,
        WebSocket
    };

    struct ToolDefinition {
        QString name;
        QString description;
        QJsonObject inputSchema;  // JSON Schema
        QString category;
        QVector<QString> tags;
        bool requiresAuth = false;
    };

    struct ResourceDefinition {
        QString uri;
        QString mimeType;
        QString description;
        bool readOnly = true;
    };

    struct MCPServerProject {
        QString name;
        QString description;
        Language language;
        TransportType transport;
        QString version;
        QVector<ToolDefinition> tools;
        QVector<ResourceDefinition> resources;
        QVector<QString> dependencies;
        QString apiBaseUrl;
        QString apiAuthType;  // "bearer", "oauth", "api_key", "basic", "none"
    };

    struct BestPractice {
        QString name;
        QString description;
        QString guidance;
        QString example;
        QString category;  // "design", "performance", "security", "testing"
    };

    SkillMCPBuilder();
    virtual ~SkillMCPBuilder() = default;

    // ── Project Generation ────────────────────────────────

    /// Generate MCP server project structure
    virtual QString generateProjectStructure(
        const MCPServerProject &project,
        const QString &outputPath
    );

    /// Generate package.json for TypeScript project
    virtual QString generatePackageJson(const MCPServerProject &project) const;

    /// Generate pyproject.toml for Python project
    virtual QString generatePyprojectToml(const MCPServerProject &project) const;

    /// Generate main server file
    virtual QString generateServerMain(const MCPServerProject &project) const;

    // ── Tool Definition ───────────────────────────────────

    /// Generate tool definition boilerplate
    virtual QString generateToolDefinition(const ToolDefinition &tool) const;

    /// Generate tool implementation template
    virtual QString generateToolImplementation(
        const ToolDefinition &tool,
        Language language
    ) const;

    /// Validate tool definition against MCP spec
    virtual bool validateToolDefinition(const ToolDefinition &tool, QString &errorMsg) const;

    /// Generate tool registry code
    virtual QString generateToolRegistry(
        const QVector<ToolDefinition> &tools,
        Language language
    ) const;

    // ── Resource Definition ───────────────────────────────

    /// Generate resource definition
    virtual QString generateResourceDefinition(const ResourceDefinition &resource) const;

    /// Generate resource handler template
    virtual QString generateResourceHandler(
        const ResourceDefinition &resource,
        Language language
    ) const;

    // ── Authentication and Configuration ──────────────────

    /// Generate authentication module
    virtual QString generateAuthModule(
        const QString &authType,
        Language language
    ) const;

    /// Generate environment configuration template
    virtual QString generateConfigTemplate(const MCPServerProject &project) const;

    /// Generate API client code
    virtual QString generateAPIClient(
        const MCPServerProject &project,
        Language language
    ) const;

    // ── Error Handling and Validation ─────────────────────

    /// Generate error handling utilities
    virtual QString generateErrorHandling(Language language) const;

    /// Generate input validation code
    virtual QString generateValidation(Language language) const;

    /// Generate logging setup
    virtual QString generateLogging(Language language) const;

    // ── Testing ───────────────────────────────────────────

    /// Generate test file for tool
    virtual QString generateTestFile(
        const ToolDefinition &tool,
        Language language
    ) const;

    /// Generate integration test template
    virtual QString generateIntegrationTest(Language language) const;

    /// Generate mock server for testing
    virtual QString generateMockServer(Language language) const;

    // ── Documentation ────────────────────────────────────

    /// Generate README.md for server
    virtual QString generateREADME(const MCPServerProject &project) const;

    /// Generate API documentation
    virtual QString generateAPIDocumentation(const MCPServerProject &project) const;

    /// Generate tool documentation
    virtual QString generateToolDocumentation(const QVector<ToolDefinition> &tools) const;

    // ── Best Practices ───────────────────────────────────

    /// Get best practices for MCP development
    virtual QVector<BestPractice> getBestPractices(const QString &category = "") const;

    /// Get performance guidelines
    virtual QString getPerformanceGuidelines() const;

    /// Get security guidelines
    virtual QString getSecurityGuidelines() const;

    /// Get scalability patterns
    virtual QString getScalabilityPatterns() const;

    // ── Common Patterns ──────────────────────────────────

    /// Generate pagination pattern
    virtual QString generatePaginationPattern(Language language) const;

    /// Generate retry logic
    virtual QString generateRetryLogic(Language language) const;

    /// Generate rate limiting
    virtual QString generateRateLimiting(Language language) const;

    /// Generate caching layer
    virtual QString generateCaching(Language language) const;

    // ── Helper Functions ────────────────────────────────

    /// Detect service type (GitHub, Slack, etc.)
    virtual QString detectServiceType(const QString &apiDescription) const;

    /// Get recommended tools for service
    virtual QVector<QString> getRecommendedToolsForService(const QString &serviceType) const;

    /// Generate tool names for service
    virtual QVector<QString> generateToolNamesForService(
        const QString &serviceType,
        int count = 5
    ) const;

    /// Convert language to string
    virtual QString languageToString(Language lang) const;

private:
    void initializeBestPractices();
    QVector<BestPractice> m_bestPractices;
};

using SkillMCPBuilderPtr = std::shared_ptr<SkillMCPBuilder>;
