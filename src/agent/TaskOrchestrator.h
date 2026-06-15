#pragma once

#include <QObject>
#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>
#include <QList>
#include <QString>
#include <QStringList>
#include <memory>

#include "agent/AgentMessage.h"
#include "agent/ContextManager.h"
#include "agent/TaskSession.h"

class TaskOrchestrator : public QObject {
    Q_OBJECT
public:
    struct StartOptions {
        QString threadId;
        QString parentThreadId;
        QString workspacePath;
        QString currentProvider;
        QString currentModel;
        QString currentFilePath;
        QString goal;
        QString status{QStringLiteral("in_progress")};
        QVariantList todoItems;
        QVariantMap approvalProfile;
        QVariantList executionTimeline;
        QVariantList contextItems;
        QList<AgentMessage> messages;
    };

    explicit TaskOrchestrator(QObject *parent = nullptr);

    void setContextManager(ContextManager *contextManager);
    void setWorkspacePath(const QString &workspacePath);
    void setCurrentProvider(const QString &providerId, const QString &modelId = {});
    void setCurrentFilePath(const QString &filePath);

    QString startTask(const QString &goal);
    QString startTask(const QString &goal, const StartOptions &options);
    QString resumeTask(const QString &threadId);
    QString forkTask(const QString &branchLabel = {});

    bool pauseTask(const QString &reason = {});
    bool completeTask(const QString &reason = {});
    bool failTask(const QString &reason = {});

    bool saveTask() const;
    bool loadTask(const QString &threadId);

    QString currentThreadId() const;
    QString parentThreadId() const;
    QString goal() const;
    QString status() const;
    TaskSessionSnapshot snapshot() const;
    QVariantList executionTimeline() const;

    QString recordUserMessage(const QString &text, const QVariantList &attachments = {});
    QString recordAssistantMessage(const AgentMessage &message);
    QString recordToolCall(const ToolCall &call, const QString &phase = QStringLiteral("started"));
    QString recordToolResult(const ToolResult &result);
    QString recordFileChange(const QString &operation, const QStringList &paths,
                             const QVariantMap &details = {});
    QString recordCheckpoint(const QString &checkpointId, const QStringList &paths,
                             const QString &description, const QVariantMap &details = {});
    QString recordContextSnapshot(const QString &label = {});

signals:
    void taskStarted(const QString &threadId, const QString &goal);
    void taskResumed(const QString &threadId);
    void taskPaused(const QString &threadId, const QString &reason);
    void taskCompleted(const QString &threadId, const QString &reason);
    void taskFailed(const QString &threadId, const QString &reason);
    void taskSaved(const QString &threadId);
    void timelineEventAdded(const QVariantMap &event);
    void contextSnapshotUpdated(const QString &snapshotId);

private:
    QString ensureThreadId(const QString &candidate) const;
    QString appendTimelineEvent(const QString &type, const QString &name,
                                const QVariantMap &data = {});
    void syncContextFromManager();
    void syncManagerFromSnapshot();
    void touchUpdatedAt();
    QVariantMap baseTimelineEvent(const QString &type, const QString &name) const;

    TaskSessionSnapshot m_snapshot;
    ContextManager *m_contextManager{nullptr};
    int m_eventCounter{0};
};
