#include "CodeChangeTracker.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QCryptographicHash>
#include <QUuid>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────────────────
// FileChange Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString FileChange::toJson() const
{
    QJsonObject obj;
    obj["filePath"] = filePath;
    obj["originalPath"] = originalPath;
    obj["changeType"] = static_cast<int>(changeType);
    obj["status"] = static_cast<int>(status);
    obj["totalAdditions"] = totalAdditions;
    obj["totalDeletions"] = totalDeletions;
    obj["totalModifications"] = totalModifications;
    obj["changedAt"] = changedAt.toString(Qt::ISODate);
    obj["stagedAt"] = stagedAt.toString(Qt::ISODate);
    obj["stageReason"] = stageReason;
    obj["authorName"] = authorName;
    obj["authorEmail"] = authorEmail;
    obj["fileHash"] = fileHash;
    obj["fileSize"] = fileSize;
    obj["fileLanguage"] = fileLanguage;
    obj["changeComplexity"] = changeComplexity;
    
    // Serialize line changes
    QJsonArray lineChangesArray;
    for (const auto &lc : lineChanges) {
        QJsonObject lcObj;
        lcObj["lineNumber"] = lc.lineNumber;
        lcObj["originalContent"] = lc.originalContent;
        lcObj["modifiedContent"] = lc.modifiedContent;
        lcObj["type"] = static_cast<int>(lc.type);
        lineChangesArray.append(lcObj);
    }
    obj["lineChanges"] = lineChangesArray;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// ──────────────────────────────────────────────────────────────────────────────
// ChangeSet Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString ChangeSet::toJson() const
{
    QJsonObject obj;
    obj["changeSetId"] = changeSetId;
    obj["branchName"] = branchName;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    obj["committedAt"] = committedAt.toString(Qt::ISODate);
    obj["commitMessage"] = commitMessage;
    obj["commitHash"] = commitHash;
    obj["authorName"] = authorName;
    obj["authorEmail"] = authorEmail;
    obj["totalFiles"] = totalFiles;
    obj["totalAdditions"] = totalAdditions;
    obj["totalDeletions"] = totalDeletions;
    obj["totalModifications"] = totalModifications;
    obj["isPushed"] = isPushed;
    obj["isMerged"] = isMerged;
    
    // Serialize file changes
    QJsonArray fileChangesArray;
    for (const auto &fc : fileChanges) {
        fileChangesArray.append(QJsonDocument::fromJson(fc.toJson().toUtf8()).object());
    }
    obj["fileChanges"] = fileChangesArray;
    
    // Serialize issues and reviewers
    QJsonArray issuesArray;
    for (const auto &issue : relatedIssues) {
        issuesArray.append(issue);
    }
    obj["relatedIssues"] = issuesArray;
    
    QJsonArray reviewersArray;
    for (const auto &reviewer : reviewers) {
        reviewersArray.append(reviewer);
    }
    obj["reviewers"] = reviewersArray;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// ──────────────────────────────────────────────────────────────────────────────
// CodeChangeTracker Implementation
// ──────────────────────────────────────────────────────────────────────────────

CodeChangeTracker::CodeChangeTracker(QObject *parent)
    : QObject(parent)
{
}

CodeChangeTracker::~CodeChangeTracker()
{
}

void CodeChangeTracker::recordChange(const FileChange &change)
{
    FileChange mutableChange = change;
    
    // Set timestamp if not set
    if (!mutableChange.changedAt.isValid()) {
        mutableChange.changedAt = QDateTime::currentDateTime();
    }
    
    // Calculate file hash
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(mutableChange.modifiedContent.toUtf8());
    mutableChange.fileHash = QString::fromLatin1(hash.result().toHex());
    
    // Set file size
    mutableChange.fileSize = mutableChange.modifiedContent.size();
    
    // Calculate complexity
    mutableChange.changeComplexity = calculateChangeComplexity(mutableChange);
    
    m_changes[mutableChange.filePath] = mutableChange;
    
    updateStatistics();
    
    emit changeRecorded(mutableChange.filePath, mutableChange.changeType);
}

void CodeChangeTracker::recordBatch(const ChangeSet &changeSet)
{
    for (const auto &change : changeSet.fileChanges) {
        recordChange(change);
    }
    
    ChangeSet mutableChangeSet = changeSet;
    if (mutableChangeSet.changeSetId.isEmpty()) {
        mutableChangeSet.changeSetId = QUuid::createUuid().toString();
    }
    if (!mutableChangeSet.createdAt.isValid()) {
        mutableChangeSet.createdAt = QDateTime::currentDateTime();
    }
    
    m_changeSets[mutableChangeSet.changeSetId] = mutableChangeSet;
    
    emit changeSetCreated(mutableChangeSet.changeSetId);
}

QVector<FileChange> CodeChangeTracker::getAllChanges() const
{
    return m_changes.values().toVector();
}

QVector<FileChange> CodeChangeTracker::getChangesByType(ChangeType type) const
{
    QVector<FileChange> result;
    for (const auto &change : m_changes) {
        if (change.changeType == type) {
            result.append(change);
        }
    }
    return result;
}

QVector<FileChange> CodeChangeTracker::getChangesByStatus(ChangeStatus status) const
{
    QVector<FileChange> result;
    for (const auto &change : m_changes) {
        if (change.status == status) {
            result.append(change);
        }
    }
    return result;
}

FileChange CodeChangeTracker::getChange(const QString &filePath) const
{
    auto it = m_changes.find(filePath);
    if (it != m_changes.end()) {
        return it.value();
    }
    return FileChange();
}

void CodeChangeTracker::stageChange(const QString &filePath, const QString &reason)
{
    auto it = m_changes.find(filePath);
    if (it != m_changes.end()) {
        it.value().status = ChangeStatus::Staged;
        it.value().stagedAt = QDateTime::currentDateTime();
        it.value().stageReason = reason;
        
        if (!m_stagedFiles.contains(filePath)) {
            m_stagedFiles.append(filePath);
        }
        
        emit changeStaged(filePath);
    }
}

void CodeChangeTracker::unstageChange(const QString &filePath)
{
    auto it = m_changes.find(filePath);
    if (it != m_changes.end()) {
        it.value().status = ChangeStatus::Unstaged;
        m_stagedFiles.removeAll(filePath);
        
        emit changeUnstaged(filePath);
    }
}

void CodeChangeTracker::stageAll()
{
    for (auto it = m_changes.begin(); it != m_changes.end(); ++it) {
        stageChange(it.key(), "Batch stage all");
    }
}

void CodeChangeTracker::unstageAll()
{
    for (auto it = m_changes.begin(); it != m_changes.end(); ++it) {
        unstageChange(it.key());
    }
}

ChangeSet CodeChangeTracker::createChangeSet(const QString &commitMessage, const QString &branchName)
{
    ChangeSet changeSet;
    changeSet.changeSetId = QUuid::createUuid().toString();
    changeSet.commitMessage = commitMessage;
    changeSet.branchName = branchName;
    changeSet.createdAt = QDateTime::currentDateTime();
    
    // Add staged files
    for (const auto &filePath : m_stagedFiles) {
        auto it = m_changes.find(filePath);
        if (it != m_changes.end()) {
            changeSet.fileChanges.append(it.value());
        }
    }
    
    // Update changeset statistics
    changeSet.totalFiles = changeSet.fileChanges.size();
    for (const auto &change : changeSet.fileChanges) {
        changeSet.totalAdditions += change.totalAdditions;
        changeSet.totalDeletions += change.totalDeletions;
        changeSet.totalModifications += change.totalModifications;
    }
    
    recordBatch(changeSet);
    
    return changeSet;
}

ChangeSet CodeChangeTracker::getChangeSet(const QString &changeSetId) const
{
    auto it = m_changeSets.find(changeSetId);
    if (it != m_changeSets.end()) {
        return it.value();
    }
    return ChangeSet();
}

QVector<ChangeSet> CodeChangeTracker::getAllChangeSets() const
{
    return m_changeSets.values().toVector();
}

QString CodeChangeTracker::getDiff(const QString &filePath) const
{
    auto it = m_changes.find(filePath);
    if (it != m_changes.end()) {
        return computeFileDiff(it.value().originalContent, it.value().modifiedContent);
    }
    return QString();
}

QString CodeChangeTracker::getDiffSummary() const
{
    QString summary;
    summary += QString("Total additions: %1\n").arg(m_totalAdditions);
    summary += QString("Total deletions: %1\n").arg(m_totalDeletions);
    summary += QString("Total modifications: %1\n").arg(m_totalModifications);
    summary += QString("Files changed: %1\n").arg(m_changes.size());
    
    return summary;
}

int CodeChangeTracker::getTotalAdditions() const
{
    return m_totalAdditions;
}

int CodeChangeTracker::getTotalDeletions() const
{
    return m_totalDeletions;
}

int CodeChangeTracker::getTotalModifications() const
{
    return m_totalModifications;
}

int CodeChangeTracker::getTotalFilesChanged() const
{
    return m_changes.size();
}

float CodeChangeTracker::calculateChangeComplexity(const FileChange &change) const
{
    // Complexity based on:
    // 1. Number of line changes
    // 2. Dispersion of changes
    // 3. File size
    
    int totalChanges = change.totalAdditions + change.totalDeletions + change.totalModifications;
    if (totalChanges == 0) {
        return 0.0f;
    }
    
    float fileSize = static_cast<float>(change.modifiedContent.size());
    float complexity = (static_cast<float>(totalChanges) / (fileSize + 1.0f)) * 100.0f;
    
    // Normalize to 0-1
    complexity = std::min(1.0f, complexity / 100.0f);
    
    return complexity;
}

bool CodeChangeTracker::saveToFile(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QJsonObject root;
    
    // Save changes
    QJsonArray changesArray;
    for (const auto &change : m_changes) {
        changesArray.append(QJsonDocument::fromJson(change.toJson().toUtf8()).object());
    }
    root["changes"] = changesArray;
    
    // Save changesets
    QJsonArray changesetsArray;
    for (const auto &changeset : m_changeSets) {
        changesetsArray.append(QJsonDocument::fromJson(changeset.toJson().toUtf8()).object());
    }
    root["changesets"] = changesetsArray;
    
    root["totalAdditions"] = m_totalAdditions;
    root["totalDeletions"] = m_totalDeletions;
    root["totalModifications"] = m_totalModifications;
    
    QJsonDocument doc(root);
    file.write(doc.toJson());
    file.close();
    
    return true;
}

bool CodeChangeTracker::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    
    if (!doc.isObject()) {
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Load statistics
    m_totalAdditions = root["totalAdditions"].toInt();
    m_totalDeletions = root["totalDeletions"].toInt();
    m_totalModifications = root["totalModifications"].toInt();
    
    return true;
}

void CodeChangeTracker::clear()
{
    m_changes.clear();
    m_changeSets.clear();
    m_stagedFiles.clear();
    m_totalAdditions = 0;
    m_totalDeletions = 0;
    m_totalModifications = 0;
}

QString CodeChangeTracker::computeFileDiff(const QString &original, const QString &modified) const
{
    QString diff;
    QStringList origLines = original.split('\n');
    QStringList modLines = modified.split('\n');
    
    diff += QString("--- Original (%1 lines)\n").arg(origLines.size());
    diff += QString("+++ Modified (%1 lines)\n").arg(modLines.size());
    diff += QString("@@ Differences @@\n");
    
    int i = 0, j = 0;
    while (i < origLines.size() || j < modLines.size()) {
        if (i < origLines.size() && j < modLines.size() && origLines[i] == modLines[j]) {
            i++;
            j++;
        } else if (i < origLines.size()) {
            diff += QString("- %1\n").arg(origLines[i]);
            i++;
        } else if (j < modLines.size()) {
            diff += QString("+ %1\n").arg(modLines[j]);
            j++;
        }
    }
    
    return diff;
}

void CodeChangeTracker::updateStatistics()
{
    m_totalAdditions = 0;
    m_totalDeletions = 0;
    m_totalModifications = 0;
    
    for (const auto &change : m_changes) {
        m_totalAdditions += change.totalAdditions;
        m_totalDeletions += change.totalDeletions;
        m_totalModifications += change.totalModifications;
    }
    
    emit statisticsUpdated();
}
