#pragma once

#include "ClaudeSkillTypes.h"
#include "SkillResourceManager.h"
#include <QString>
#include <QMap>
#include <QVector>
#include <memory>

/**
 * @class SkillAPIReference
 * @brief Claude API Reference Skill
 * 
 * Provides multi-language SDK reference and documentation for:
 * - Model pricing and capabilities
 * - SDK usage patterns
 * - API migration guides
 * - Error handling best practices
 * - Tool use and streaming examples
 * 
 * Supported Languages:
 * - Python, TypeScript, Java, Go, Ruby, C#, PHP, cURL
 */
class SkillAPIReference {
public:
    enum class ProgrammingLanguage {
        Python,
        TypeScript,
        Java,
        Go,
        Ruby,
        CSharp,
        PHP,
        cURL
    };

    struct ModelInfo {
        QString modelId;
        QString displayName;
        QString description;
        int maxTokens;
        QString releaseDate;
        double pricePerMTok;  // Price per million tokens
        double pricePerOutputMTok;
        QString family;
        QVector<QString> capabilities;
        bool available;
    };

    struct SDKReference {
        ProgrammingLanguage language;
        QString languageName;
        QString sdkName;
        QString officialRepo;
        QString latestVersion;
        QVector<QString> features;
        QString installCommand;
    };

    struct APIPattern {
        QString name;
        QString description;
        QString category;  // "streaming", "tool-use", "vision", "caching", etc.
        QString codeExample;
        QVector<QString> relatedModels;
    };

    SkillAPIReference();
    virtual ~SkillAPIReference() = default;

    // ── Model Information ─────────────────────────────────

    /// Get information about available Claude models
    virtual QVector<ModelInfo> getAvailableModels() const;

    /// Get information about specific model
    virtual ModelInfo getModelInfo(const QString &modelId) const;

    /// Get latest stable model recommendation
    virtual QString getLatestRecommendedModel() const;

    /// Get model pricing information
    virtual QString getPricingInfo(const QString &modelId) const;

    // ── SDK Reference ────────────────────────────────────

    /// Get SDK reference for specific language
    virtual SDKReference getSDKReference(ProgrammingLanguage language) const;

    /// Get installation command for SDK
    virtual QString getInstallCommand(ProgrammingLanguage language) const;

    /// Get SDK quick start code
    virtual QString getQuickStart(ProgrammingLanguage language) const;

    // ── API Patterns and Examples ────────────────────────

    /// Get API pattern examples
    virtual QVector<APIPattern> getAPIPatterns(const QString &category = "") const;

    /// Get code example for specific pattern
    virtual QString getPatternExample(
        const QString &patternName,
        ProgrammingLanguage language
    ) const;

    /// Get migration guide from older model/version
    virtual QString getMigrationGuide(
        const QString &fromModel,
        const QString &toModel,
        ProgrammingLanguage language
    ) const;

    // ── Tool Use Documentation ───────────────────────────

    /// Get tool use/function calling reference
    virtual QString getToolUseReference(ProgrammingLanguage language) const;

    /// Get tool definition examples (JSON schema)
    virtual QString getToolDefinitionExamples() const;

    // ── Streaming and Advanced Features ──────────────────

    /// Get streaming implementation guide
    virtual QString getStreamingGuide(ProgrammingLanguage language) const;

    /// Get vision capabilities documentation
    virtual QString getVisionGuide(ProgrammingLanguage language) const;

    /// Get prompt caching guide
    virtual QString getPromptCachingGuide(ProgrammingLanguage language) const;

    /// Get batch processing guide
    virtual QString getBatchProcessingGuide(ProgrammingLanguage language) const;

    // ── Error Handling and Best Practices ────────────────

    /// Get error handling guide
    virtual QString getErrorHandlingGuide(ProgrammingLanguage language) const;

    /// Get best practices documentation
    virtual QString getBestPractices() const;

    /// Get rate limiting and retry strategies
    virtual QString getRateLimitingGuide() const;

    // ── Language Detection and Helpers ──────────────────

    /// Detect programming language from file content/extension
    virtual ProgrammingLanguage detectLanguage(const QString &fileContent, const QString &fileName = "") const;

    /// Convert language enum to string
    virtual QString languageToString(ProgrammingLanguage lang) const;

    /// Convert string to language enum
    virtual ProgrammingLanguage stringToLanguage(const QString &langStr) const;

private:
    void initializeModels();
    void initializeSDKReferences();
    void initializeAPIPatterns();

    QVector<ModelInfo> m_models;
    QMap<ProgrammingLanguage, SDKReference> m_sdkReferences;
    QVector<APIPattern> m_apiPatterns;
};

using SkillAPIReferencePtr = std::shared_ptr<SkillAPIReference>;
