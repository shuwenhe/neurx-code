#pragma once

#include "SkillAPIReference.h"
#include "SkillMCPBuilder.h"
#include "SkillCreator.h"
#include "SkillResourceManager.h"
#include <QString>
#include <QObject>
#include <memory>

/**
 * @class SkillsRegistry
 * @brief Central registry for Claude Code Skills implementation
 * 
 * Provides unified access to all skill implementations:
 * - SkillAPIReference - Claude API and SDK documentation
 * - SkillMCPBuilder - MCP server development framework
 * - SkillCreator - Skill creation and evaluation tools
 * - SkillResourceManager - Resource loading and management
 */
class SkillsRegistry : public QObject {
    Q_OBJECT

public:
    SkillsRegistry(QObject *parent = nullptr);
    virtual ~SkillsRegistry() = default;

    // ── Initialization ────────────────────────────────────

    /// Initialize all skills with resource directory
    virtual void initialize(const QString &resourcesPath);

    // ── Skill Access ──────────────────────────────────────

    /// Get API Reference skill
    virtual SkillAPIReferencePtr getAPIReferenceSkill() const { return m_apiReference; }

    /// Get MCP Builder skill
    virtual SkillMCPBuilderPtr getMCPBuilder() const { return m_mcpBuilder; }

    /// Get Skill Creator
    virtual SkillCreatorPtr getSkillCreator() const { return m_skillCreator; }

    /// Get Resource Manager
    virtual SkillResourceManagerPtr getResourceManager() const { return m_resourceManager; }

    // ── Skill Discovery ───────────────────────────────────

    /// Get list of all registered skills
    virtual QVector<QString> getAvailableSkills() const;

    /// Get skill description
    virtual QString getSkillDescription(const QString &skillId) const;

    /// Get skill capabilities
    virtual QVector<QString> getSkillCapabilities(const QString &skillId) const;

    // ── Unified Operations ────────────────────────────────

    /// Search across all skills
    virtual QVector<QString> searchSkills(const QString &query, int maxResults = 10) const;

    /// Get skill context for LLM
    virtual QString getSkillContextForLLM(const QString &skillId, int tier = 2) const;

    // ── Statistics ────────────────────────────────────────

    /// Get skill usage statistics
    virtual QMap<QString, int> getSkillStats() const;

    /// Get total loaded skills
    virtual int getTotalSkillsLoaded() const { return 4; }

private:
    SkillAPIReferencePtr m_apiReference;
    SkillMCPBuilderPtr m_mcpBuilder;
    SkillCreatorPtr m_skillCreator;
    SkillResourceManagerPtr m_resourceManager;
    QString m_resourcesPath;
};

using SkillsRegistryPtr = std::shared_ptr<SkillsRegistry>;
