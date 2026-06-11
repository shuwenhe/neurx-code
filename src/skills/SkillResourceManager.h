#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <memory>
#include <functional>

/**
 * @class SkillResourceManager
 * @brief Manages skill resources including scripts, templates, and documentation
 * 
 * Provides:
 * - Resource loading (scripts, templates, docs)
 * - Caching for performance
 * - Script execution management
 * - Template variable substitution
 */
class SkillResourceManager {
public:
    enum class ResourceType {
        Script,      // Python, shell scripts
        Template,    // HTML, code templates
        Reference,   // Markdown documentation
        Asset        // Images, fonts, data files
    };

    struct SkillResource {
        QString name;
        ResourceType type;
        QString path;
        QString content;
        QMap<QString, QString> metadata;
        bool cached = false;
    };

    struct ScriptExecution {
        QString skillId;
        QString scriptName;
        QStringList arguments;
        QString workingDirectory;
        QMap<QString, QString> environment;
        int timeoutMs = 30000;
        bool captureOutput = true;
    };

    struct ScriptResult {
        bool success = false;
        int exitCode = 0;
        QString stdout;
        QString stderr;
        QString error;
        qint64 executionTimeMs = 0;
    };

    SkillResourceManager();
    virtual ~SkillResourceManager() = default;

    /// Override the base path used to discover skill resources.
    void setSkillsBasePath(const QString &basePath);
    QString skillsBasePath() const;

    // ── Resource Loading ──────────────────────────────────
    
    /// Load skill resource from filesystem
    virtual SkillResource loadResource(
        const QString &skillId,
        const QString &resourceName,
        ResourceType type
    );

    /// Load all resources for a skill
    virtual QVector<SkillResource> loadSkillResources(const QString &skillId);

    /// Get cached resource (returns empty if not cached)
    virtual SkillResource getCachedResource(
        const QString &skillId,
        const QString &resourceName,
        ResourceType type
    );

    // ── Script Execution ──────────────────────────────────
    
    /// Execute a skill script synchronously
    virtual ScriptResult executeScript(const ScriptExecution &execution);

    /// Execute a skill script asynchronously
    virtual void executeScriptAsync(
        const ScriptExecution &execution,
        std::function<void(const ScriptResult &)> callback
    );

    // ── Template Processing ───────────────────────────────
    
    /// Process template with variable substitution
    virtual QString processTemplate(
        const QString &template_content,
        const QMap<QString, QString> &variables
    );

    /// Load and process template file
    virtual QString loadAndProcessTemplate(
        const QString &skillId,
        const QString &templateName,
        const QMap<QString, QString> &variables
    );

    // ── Cache Management ──────────────────────────────────
    
    /// Clear resource cache for specific skill
    virtual void clearCache(const QString &skillId);

    /// Clear all caches
    virtual void clearAllCaches();

    /// Get cache statistics
    virtual QMap<QString, int> getCacheStats() const;

private:
    QMap<QString, QVector<SkillResource>> m_resourceCache;
    QString m_skillsBasePath;
};

using SkillResourceManagerPtr = std::shared_ptr<SkillResourceManager>;
