#pragma once

#include "agent/AgentToolRegistry.h"
#include "tools/CheckpointManager.h"
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <memory>

/**
 * @class AgentFileWriterTool
 * @brief Specialized tool for Agent-based file writing operations
 * 
 * This tool provides comprehensive file writing capabilities for the neurx-code agent,
 * including:
 * - Single file writing with atomic operations
 * - Batch file writing for multiple files
 * - Template-based file generation
 * - Incremental content updates (append, prepend, insert)
 * - Automatic parent directory creation
 * - Checkpoint/backup support
 * - Content validation before write
 * 
 * Usage Example (Agent execution):
 * 
 * Agent Plan:
 *   1. Write main.py with CLI setup
 *   2. Write requirements.txt with dependencies
 *   3. Write README.md with documentation
 * 
 * Tool Call:
 *   {
 *     "operation": "write_single",
 *     "path": "src/main.py",
 *     "content": "#!/usr/bin/env python3\n...",
 *     "create_dirs": true,
 *     "backup": true
 *   }
 */
class AgentFileWriterTool : public BaseTool {
    Q_OBJECT

public:
    explicit AgentFileWriterTool(const QString &workspaceRoot, QObject *parent = nullptr);

    QString name() const override { return "agent_file_writer"; }
    QString description() const override {
        return "Write, update, and manage files from agent execution. Supports single/batch writes, "
               "templates, atomic operations, and checkpoints. Use when agent needs to create/modify files.";
    }

    QJsonObject parametersSchema() const override;
    ToolResult execute(const QString &callId, const QJsonObject &args) override;
    QString summary(const QJsonObject &args) const override;

private:
    // ── Operation Handlers ─────────────────────────────────────────────────────

    /// Write a single file with content
    ToolResult opWriteSingle(const QString &callId, const QJsonObject &args);

    /// Write multiple files in batch (atomic: all-or-nothing)
    ToolResult opWriteBatch(const QString &callId, const QJsonObject &args);

    /// Update file content (append, prepend, or insert at line)
    ToolResult opUpdateFile(const QString &callId, const QJsonObject &args);

    /// Generate file from template with variable substitution
    ToolResult opWriteTemplate(const QString &callId, const QJsonObject &args);

    /// Create a file structure from JSON schema (directory tree)
    ToolResult opCreateStructure(const QString &callId, const QJsonObject &args);

    // ── Helper Methods ────────────────────────────────────────────────────────

    /// Validate path is within workspace (prevent traversal attacks)
    QString safePath(const QString &relativePath) const;

    /// Write content to file atomically (all-or-nothing)
    bool writeFileAtomically(const QString &filePath, const QString &content, QString &errorMsg);

    /// Create parent directories if needed
    bool ensureDirectories(const QString &filePath, QString &errorMsg);

    /// Create backup of existing file
    QString createBackup(const QString &filePath);

    /// Validate content before writing (file size, encoding, etc.)
    bool validateContent(const QString &content, QString &errorMsg) const;

    /// Apply variable substitution to template
    QString processTemplate(const QString &templateContent, const QJsonObject &variables) const;

    // ── Data Members ────────────────────────────────────────────────────────

    QString m_workspaceRoot;
    std::unique_ptr<CheckpointManager> m_checkpointManager;

    // Configuration
    static constexpr qint64 MAX_FILE_SIZE = 10 * 1024 * 1024;  // 10 MB
    static constexpr int MAX_BATCH_FILES = 100;
};
