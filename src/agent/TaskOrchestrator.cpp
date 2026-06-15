#include "agent/TaskOrchestrator.h"

#include <QUuid>
#include <QDir>
#include <QJsonArray>
#include <QJsonObject>

namespace {

QString normalizeId(const QString &value)
{
    return value.trimmed();
}

QString generateTimelineId(int counter)
{
    return QStringLiteral("timeline_%1_%2")
        .arg(counter)
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

} // namespace

TaskOrchestrator::TaskOrchestrator(QObject *parent)
    : QObject(parent)
{
}

void TaskOrchestrator::setContextManager(ContextManager *contextManager)
{
    m_contextManager = contextManager;
    syncContextFromManager();
}

void TaskOrchestrator::setWorkspacePath(const QString &workspacePath)
{
    m_snapshot.workspacePath = workspacePath.trimmed();
    touchUpdatedAt();
}

void TaskOrchestrator::setCurrentProvider(const QString &providerId, const QString &modelId)
{
    m_snapshot.currentProvider = providerId.trimmed();
    if (!modelId.trimmed().isEmpty()) {
        m_snapshot.currentModel = modelId.trimmed();
    }
    touchUpdatedAt();
}

void TaskOrchestrator::setCurrentFilePath(const QString &filePath)
{
    m_snapshot.currentFilePath = filePath.trimmed();
    touchUpdatedAt();
}

QString TaskOrchestrator::startTask(const QString &goal, const StartOptions &options)
{
    m_snapshot = TaskSessionSnapshot{};
    m_snapshot.threadId = ensureThreadId(options.threadId);
    m_snapshot.sessionId = m_snapshot.threadId;
    m_snapshot.parentThreadId = normalizeId(options.parentThreadId);
    m_snapshot.goal = goal.trimmed();
    m_snapshot.status = options.status.trimmed().isEmpty()
        ? QStringLiteral("in_progress")
        : options.status.trimmed();
    m_snapshot.workspacePath = options.workspacePath.trimmed();
    m_snapshot.currentProvider = options.currentProvider.trimmed();
    m_snapshot.currentModel = options.currentModel.trimmed();
    m_snapshot.currentFilePath = options.currentFilePath.trimmed();
    m_snapshot.todoItems = options.todoItems;
    m_snapshot.approvalProfile = options.approvalProfile;
    m_snapshot.executionTimeline = options.executionTimeline;
    m_snapshot.contextItems = options.contextItems;
    m_snapshot.messages = options.messages;
    m_snapshot.updatedAt = QDateTime::currentDateTimeUtc();

    syncContextFromManager();
    saveTask();
    emit taskStarted(m_snapshot.threadId, m_snapshot.goal);
    return m_snapshot.threadId;
}

QString TaskOrchestrator::startTask(const QString &goal)
{
    return startTask(goal, StartOptions{});
}

QString TaskOrchestrator::resumeTask(const QString &threadId)
{
    if (!loadTask(threadId)) {
        return {};
    }

    m_snapshot.threadId = ensureThreadId(m_snapshot.threadId);
    m_snapshot.sessionId = m_snapshot.threadId;
    m_snapshot.status = QStringLiteral("in_progress");
    syncManagerFromSnapshot();
    saveTask();
    emit taskResumed(m_snapshot.threadId);
    return m_snapshot.threadId;
}

QString TaskOrchestrator::forkTask(const QString &branchLabel)
{
    const QString sourceThreadId = currentThreadId();
    if (sourceThreadId.isEmpty()) {
        return {};
    }

    StartOptions options;
    options.parentThreadId = sourceThreadId;
    options.workspacePath = m_snapshot.workspacePath;
    options.currentProvider = m_snapshot.currentProvider;
    options.currentModel = m_snapshot.currentModel;
    options.currentFilePath = m_snapshot.currentFilePath;
    options.goal = m_snapshot.goal;
    options.status = QStringLiteral("in_progress");
    options.todoItems = m_snapshot.todoItems;
    options.approvalProfile = m_snapshot.approvalProfile;
    options.executionTimeline = m_snapshot.executionTimeline;
    options.contextItems = m_snapshot.contextItems;
    options.messages = m_snapshot.messages;

    const QString suffix = branchLabel.trimmed().isEmpty()
        ? QStringLiteral("branch")
        : branchLabel.trimmed();
    options.threadId = QStringLiteral("%1-%2-%3")
        .arg(sourceThreadId, suffix, QUuid::createUuid().toString(QUuid::WithoutBraces));
    return startTask(m_snapshot.goal, options);
}

bool TaskOrchestrator::pauseTask(const QString &reason)
{
    if (currentThreadId().isEmpty()) {
        return false;
    }
    m_snapshot.status = QStringLiteral("paused");
    appendTimelineEvent(QStringLiteral("task"), QStringLiteral("paused"),
                        {{QStringLiteral("reason"), reason}});
    saveTask();
    emit taskPaused(currentThreadId(), reason);
    return true;
}

bool TaskOrchestrator::completeTask(const QString &reason)
{
    if (currentThreadId().isEmpty()) {
        return false;
    }
    m_snapshot.status = QStringLiteral("completed");
    appendTimelineEvent(QStringLiteral("task"), QStringLiteral("completed"),
                        {{QStringLiteral("reason"), reason}});
    saveTask();
    emit taskCompleted(currentThreadId(), reason);
    return true;
}

bool TaskOrchestrator::failTask(const QString &reason)
{
    if (currentThreadId().isEmpty()) {
        return false;
    }
    m_snapshot.status = QStringLiteral("failed");
    appendTimelineEvent(QStringLiteral("task"), QStringLiteral("failed"),
                        {{QStringLiteral("reason"), reason}});
    saveTask();
    emit taskFailed(currentThreadId(), reason);
    return true;
}

bool TaskOrchestrator::saveTask() const
{
    TaskSessionSnapshot snapshot = m_snapshot;
    if (snapshot.threadId.trimmed().isEmpty()) {
        return false;
    }

    snapshot.sessionId = ensureThreadId(snapshot.sessionId);
    snapshot.threadId = ensureThreadId(snapshot.threadId);
    snapshot.updatedAt = QDateTime::currentDateTimeUtc();

    return TaskSessionStore::saveLatest(snapshot);
}

bool TaskOrchestrator::loadTask(const QString &threadId)
{
    const QString normalized = ensureThreadId(threadId);
    if (normalized.isEmpty()) {
        return false;
    }

    const TaskSessionSnapshot loaded = TaskSessionStore::loadById(normalized);
    if (!loaded.isValid()) {
        return false;
    }

    m_snapshot = loaded;
    m_snapshot.threadId = ensureThreadId(m_snapshot.threadId);
    m_snapshot.sessionId = ensureThreadId(m_snapshot.sessionId);
    syncManagerFromSnapshot();
    return true;
}

QString TaskOrchestrator::currentThreadId() const
{
    return ensureThreadId(m_snapshot.threadId);
}

QString TaskOrchestrator::parentThreadId() const
{
    return normalizeId(m_snapshot.parentThreadId);
}

QString TaskOrchestrator::goal() const
{
    return m_snapshot.goal;
}

QString TaskOrchestrator::status() const
{
    return m_snapshot.status;
}

TaskSessionSnapshot TaskOrchestrator::snapshot() const
{
    return m_snapshot;
}

QVariantList TaskOrchestrator::executionTimeline() const
{
    return m_snapshot.executionTimeline;
}

QString TaskOrchestrator::recordUserMessage(const QString &text, const QVariantList &attachments)
{
    AgentMessage message;
    message.role = MessageRole::User;
    message.content = text;
    message.attachments = attachments;
    message.timestamp = QDateTime::currentDateTimeUtc();
    m_snapshot.messages.append(message);
    const QString eventId = appendTimelineEvent(
        QStringLiteral("message"),
        QStringLiteral("user"),
        {{QStringLiteral("content"), text},
         {QStringLiteral("attachmentCount"), attachments.size()}});
    touchUpdatedAt();
    saveTask();
    return eventId;
}

QString TaskOrchestrator::recordAssistantMessage(const AgentMessage &message)
{
    AgentMessage stored = message;
    stored.timestamp = QDateTime::currentDateTimeUtc();
    m_snapshot.messages.append(stored);
    appendTimelineEvent(QStringLiteral("message"), QStringLiteral("assistant"),
                        {{QStringLiteral("content"), stored.content},
                         {QStringLiteral("toolCallCount"), stored.toolCalls.size()},
                         {QStringLiteral("toolResultCount"), stored.toolResults.size()}});
    touchUpdatedAt();
    saveTask();
    return stored.content;
}

QString TaskOrchestrator::recordToolCall(const ToolCall &call, const QString &phase)
{
    return appendTimelineEvent(QStringLiteral("tool"), phase,
                               {{QStringLiteral("callId"), call.id},
                                {QStringLiteral("name"), call.name},
                                {QStringLiteral("arguments"), call.arguments}});
}

QString TaskOrchestrator::recordToolResult(const ToolResult &result)
{
    m_snapshot.executionTimeline.append(QVariantMap{
        {QStringLiteral("id"), generateTimelineId(++m_eventCounter)},
        {QStringLiteral("type"), QStringLiteral("tool")},
        {QStringLiteral("name"), result.name},
        {QStringLiteral("phase"), QStringLiteral("result")},
        {QStringLiteral("callId"), result.callId},
        {QStringLiteral("isError"), result.isError},
        {QStringLiteral("content"), result.content},
        {QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}
    });
    emit timelineEventAdded(m_snapshot.executionTimeline.back().toMap());
    touchUpdatedAt();
    saveTask();
    return result.content;
}

QString TaskOrchestrator::recordFileChange(const QString &operation, const QStringList &paths,
                                           const QVariantMap &details)
{
    QVariantMap data = details;
    data[QStringLiteral("paths")] = paths;
    return appendTimelineEvent(QStringLiteral("file_change"), operation, data);
}

QString TaskOrchestrator::recordCheckpoint(const QString &checkpointId, const QStringList &paths,
                                           const QString &description, const QVariantMap &details)
{
    QVariantMap data = details;
    data[QStringLiteral("checkpointId")] = checkpointId;
    data[QStringLiteral("paths")] = paths;
    data[QStringLiteral("description")] = description;
    return appendTimelineEvent(QStringLiteral("checkpoint"), QStringLiteral("created"), data);
}

QString TaskOrchestrator::recordContextSnapshot(const QString &label)
{
    if (m_contextManager) {
        m_snapshot.contextItems = m_contextManager->exportContextItems();
    }

    const QString snapshotId = appendTimelineEvent(
        QStringLiteral("context"),
        QStringLiteral("snapshot"),
        {{QStringLiteral("label"), label},
         {QStringLiteral("itemCount"), m_snapshot.contextItems.size()}});
    emit contextSnapshotUpdated(snapshotId);
    saveTask();
    return snapshotId;
}

QString TaskOrchestrator::ensureThreadId(const QString &candidate) const
{
    const QString normalized = candidate.trimmed();
    if (!normalized.isEmpty()) {
        return normalized;
    }
    return QString();
}

QString TaskOrchestrator::appendTimelineEvent(const QString &type, const QString &name,
                                              const QVariantMap &data)
{
    ++m_eventCounter;
    QVariantMap event = baseTimelineEvent(type, name);
    for (auto it = data.begin(); it != data.end(); ++it) {
        event.insert(it.key(), it.value());
    }
    m_snapshot.executionTimeline.append(event);
    emit timelineEventAdded(event);
    touchUpdatedAt();
    return event.value(QStringLiteral("id")).toString();
}

void TaskOrchestrator::syncContextFromManager()
{
    if (!m_contextManager) {
        return;
    }

    m_snapshot.contextItems = m_contextManager->exportContextItems();
}

void TaskOrchestrator::syncManagerFromSnapshot()
{
    if (!m_contextManager) {
        return;
    }

    if (!m_snapshot.contextItems.isEmpty()) {
        m_contextManager->importContextItems(m_snapshot.contextItems, true);
    }
}

void TaskOrchestrator::touchUpdatedAt()
{
    m_snapshot.updatedAt = QDateTime::currentDateTimeUtc();
}

QVariantMap TaskOrchestrator::baseTimelineEvent(const QString &type, const QString &name) const
{
    return QVariantMap{
        {QStringLiteral("id"), generateTimelineId(m_eventCounter)},
        {QStringLiteral("type"), type},
        {QStringLiteral("name"), name},
        {QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}
    };
}
