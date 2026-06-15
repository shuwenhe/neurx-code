#pragma once
#include "agent/AgentToolRegistry.h"
#include "sandbox/SandboxManager.h"
#include "tools/CheckpointManager.h"
#include "tools/PatchTool.h"
#include "tools/SmartFileCreator.h"
#include <QDir>
#include <memory>

// ── FileSystemTool ────────────────────────────────────────────────────────────
//  Provides read_file, write_file, list_directory, create_file, delete_file, move_file,
//  and copy_file. Operations are sandboxed to the configured workspace root.

class FileSystemTool : public BaseTool {
    Q_OBJECT
public:
    explicit FileSystemTool(const QString &workspaceRoot, QObject *parent = nullptr);

    QString name()        const override { return "file_system"; }
    QString description() const override {
        return "Read, write, list, and manage files within the workspace. "
               "Operations: read_file, write_file, list_directory, create_file, "
               "delete_file, move_file, copy_file, get_metadata, stat_file, "
               "hash_file, get_permissions, chmod, find_files, search_text, "
               "symlink, touch, truncate, read_range, tail, write_batch, exists, "
               "create_directory, remove, canonicalize, "
               "join, parent, append, diff, replace, replace_batch, read_many, tree, copy_tree, move_tree, watch, unwatch.";
    }
    QJsonObject parametersSchema() const override;
    ToolResult  execute(const QString &callId, const QJsonObject &args) override;
    QString     summary(const QJsonObject &args) const override;

    void setSandboxManager(SandboxManager *manager);

private:
    bool isWriteOperation(const QString &operation) const;
    ToolResult opReadFile(const QString &callId, const QJsonObject &args);
    ToolResult opWriteFile(const QString &callId, const QJsonObject &args);
    ToolResult opListDir(const QString &callId, const QJsonObject &args);
    ToolResult opCreateFile(const QString &callId, const QJsonObject &args);
    ToolResult opDeleteFile(const QString &callId, const QJsonObject &args);
    ToolResult opMoveFile(const QString &callId, const QJsonObject &args);
    ToolResult opRenameFile(const QString &callId, const QJsonObject &args);
    ToolResult opCopyFile(const QString &callId, const QJsonObject &args);
    ToolResult opGetMetadata(const QString &callId, const QJsonObject &args);
    ToolResult opStatFile(const QString &callId, const QJsonObject &args);
    ToolResult opHashFile(const QString &callId, const QJsonObject &args);
    ToolResult opGetPermissions(const QString &callId, const QJsonObject &args);
    ToolResult opChmodFile(const QString &callId, const QJsonObject &args);
    ToolResult opSymlinkFile(const QString &callId, const QJsonObject &args);
    ToolResult opTouchFile(const QString &callId, const QJsonObject &args);
    ToolResult opTruncateFile(const QString &callId, const QJsonObject &args);
    ToolResult opReadRangeFile(const QString &callId, const QJsonObject &args);
    ToolResult opTailFile(const QString &callId, const QJsonObject &args);
    ToolResult opWriteBatch(const QString &callId, const QJsonObject &args);
    ToolResult opExists(const QString &callId, const QJsonObject &args);
    ToolResult opFindFiles(const QString &callId, const QJsonObject &args);
    ToolResult opSearchText(const QString &callId, const QJsonObject &args);
    ToolResult opAppendFile(const QString &callId, const QJsonObject &args);
    ToolResult opDiffFiles(const QString &callId, const QJsonObject &args);
    ToolResult opReplaceText(const QString &callId, const QJsonObject &args);
    ToolResult opReplaceBatch(const QString &callId, const QJsonObject &args);
    ToolResult opReadMany(const QString &callId, const QJsonObject &args);
    ToolResult opPreviewPatch(const QString &callId, const QJsonObject &args);
    ToolResult opApplyPatch(const QString &callId, const QJsonObject &args);
    ToolResult opTree(const QString &callId, const QJsonObject &args);
    ToolResult opCopyTree(const QString &callId, const QJsonObject &args);
    ToolResult opMoveTree(const QString &callId, const QJsonObject &args);
    ToolResult opWatchFile(const QString &callId, const QJsonObject &args);
    ToolResult opUnwatchFile(const QString &callId, const QJsonObject &args);
    ToolResult opCreateDirectory(const QString &callId, const QJsonObject &args);
    ToolResult opRemove(const QString &callId, const QJsonObject &args);
    ToolResult opCanonicalize(const QString &callId, const QJsonObject &args);
    ToolResult opJoin(const QString &callId, const QJsonObject &args);
    ToolResult opParent(const QString &callId, const QJsonObject &args);

    // Resolve a user-supplied path against workspaceRoot; returns empty on traversal attack.
    QString safePath(const QString &relOrAbsPath) const;
    QString workspaceRelativePath(const QString &relOrAbsPath) const;
    QString checkpointPaths(const QStringList &paths, const QString &description) const;

    QDir m_root;
    std::unique_ptr<CheckpointManager> m_checkpointManager;
    std::unique_ptr<PatchTool> m_patchTool;
    std::unique_ptr<SmartFileCreator> m_smartFileCreator;
    SandboxManager *m_sandboxManager{nullptr};
};
