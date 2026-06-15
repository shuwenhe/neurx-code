#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <memory>

/**
 * @class CodeChangeTracker
 * @brief Tracks and manages code changes with version control integration
 * 
 * Features:
 * - Track file changes (created, modified, deleted)
 * - Diff computation and storage
 * - Change history with timestamps
 * - Commit metadata association
 * - Change staging/unstaging
 */

// ──────────────────────────────────────────────────────────────────────────────
// Code Change Types
// ──────────────────────────────────────────────────────────────────────────────

enum class ChangeType {
    Created,                // New file created
    Modified,               // File modified
    Deleted,                // File deleted
    Renamed,                // File renamed
    ModeChanged             // File permissions/mode changed
};

enum class ChangeStatus {
    Staged,                 // Ready for commit
    Unstaged,               // Not yet staged
    Ignored,                // Explicitly ignored
    Conflicted,             // Merge conflict
    Reverted                // Change reverted
};

// ──────────────────────────────────────────────────────────────────────────────
// Line Change Information
// ──────────────────────────────────────────────────────────────────────────────

struct LineChange {
    int lineNumber{0};
    QString originalContent;
    QString modifiedContent;
    ChangeType type{ChangeType::Modified};
    QDateTime changedAt;
};

// ──────────────────────────────────────────────────────────────────────────────
// File Change Record
// ──────────────────────────────────────────────────────────────────────────────

struct FileChange {
    QString filePath;
    QString originalPath;               // For renames
    ChangeType changeType{ChangeType::Modified};
    ChangeStatus status{ChangeStatus::Unstaged};
    
    QString originalContent;            // Full original content
    QString modifiedContent;            // Full modified content
    
    QVector<LineChange> lineChanges;    // Per-line changes
    int totalAdditions{0};
    int totalDeletions{0};
    int totalModifications{0};
    
    QDateTime changedAt;
    QDateTime stagedAt;
    QString stageReason;                // Why this was staged
    
    QString authorName;
    QString authorEmail;
    
    // Metadata
    QString fileHash;                   // SHA-1 hash of modified content
    int fileSize{0};
    QString fileLanguage;               // Detected language
    
    // Statistics
    int contextLines{3};                // Lines of context around changes
    float changeComplexity{0.0f};       // 0-1 complexity score
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Change Batch/Changeset
// ──────────────────────────────────────────────────────────────────────────────

struct ChangeSet {
    QString changeSetId;
    QString branchName;
    
    QVector<FileChange> fileChanges;
    
    QDateTime createdAt;
    QDateTime committedAt;
    
    QString commitMessage;
    QString commitHash;                 // Git commit SHA
    
    QString authorName;
    QString authorEmail;
    
    int totalFiles{0};
    int totalAdditions{0};
    int totalDeletions{0};
    int totalModifications{0};
    
    bool isPushed{false};
    bool isMerged{false};
    
    QStringList relatedIssues;          // GitHub/GitLab issues
    QStringList reviewers;              // Assigned reviewers
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Code Change Tracker
// ──────────────────────────────────────────────────────────────────────────────

class CodeChangeTracker : public QObject {
    Q_OBJECT

public:
    explicit CodeChangeTracker(QObject *parent = nullptr);
    ~CodeChangeTracker();

    // Change recording
    void recordChange(const FileChange &change);
    void recordBatch(const ChangeSet &changeSet);
    
    // Change querying
    QVector<FileChange> getAllChanges() const;
    QVector<FileChange> getChangesByType(ChangeType type) const;
    QVector<FileChange> getChangesByStatus(ChangeStatus status) const;
    FileChange getChange(const QString &filePath) const;
    
    // Change staging/unstaging
    void stageChange(const QString &filePath, const QString &reason = "");
    void unstageChange(const QString &filePath);
    void stageAll();
    void unstageAll();
    
    // Changeset management
    ChangeSet createChangeSet(const QString &commitMessage, const QString &branchName = "main");
    ChangeSet getChangeSet(const QString &changeSetId) const;
    QVector<ChangeSet> getAllChangeSets() const;
    
    // Diff operations
    QString getDiff(const QString &filePath) const;
    QString getDiffSummary() const;
    
    // Statistics
    int getTotalAdditions() const;
    int getTotalDeletions() const;
    int getTotalModifications() const;
    int getTotalFilesChanged() const;
    
    float calculateChangeComplexity(const FileChange &change) const;
    
    // Persistence
    bool saveToFile(const QString &filePath) const;
    bool loadFromFile(const QString &filePath);
    
    void clear();

signals:
    void changeRecorded(const QString &filePath, ChangeType type);
    void changeStaged(const QString &filePath);
    void changeUnstaged(const QString &filePath);
    void changeSetCreated(const QString &changeSetId);
    void statisticsUpdated();

private:
    QString computeFileDiff(const QString &original, const QString &modified) const;
    void updateStatistics();
    
    // Member variables
    QMap<QString, FileChange> m_changes;
    QMap<QString, ChangeSet> m_changeSets;
    
    int m_totalAdditions{0};
    int m_totalDeletions{0};
    int m_totalModifications{0};
    
    QStringList m_stagedFiles;
};
