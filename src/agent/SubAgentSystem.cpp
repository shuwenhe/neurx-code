#include "SubAgentSystem.h"
#include "agent/AgentEngine.h"
#include "llm/LLMProvider.h"
#include <QTimer>
#include <QThread>
#include <QDateTime>
#include <QUuid>
#include <algorithm>

SubAgentSystem::SubAgentSystem(QObject *parent)
    : QObject(parent)
{
    // Setup cleanup timer (runs every 30 seconds)
    QTimer *cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, &SubAgentSystem::cleanupIdleAgents);
    cleanupTimer->start(30000);

    // Setup health check timer (every 60 seconds)
    QTimer *healthCheckTimer = new QTimer(this);
    connect(healthCheckTimer, &QTimer::timeout, this, &SubAgentSystem::healthCheck);
    healthCheckTimer->start(60000);
}

SubAgentSystem::~SubAgentSystem()
{
    terminateAllSubAgents();
}

void SubAgentSystem::setLLMProvider(LLMProvider *provider)
{
    m_provider = provider;
}

void SubAgentSystem::setParentAgentEngine(AgentEngine *parent)
{
    m_parentEngine = parent;
}

void SubAgentSystem::registerAgentType(const SubAgentConfig &config)
{
    m_agentTypeConfigs[config.agentType] = config;
}

QString SubAgentSystem::spawnSubAgent(const QString &agentType, const QString &expertise)
{
    if (!m_agentTypeConfigs.contains(agentType)) {
        qWarning() << "Agent type not registered:" << agentType;
        return "";
    }

    QString agentId = QString("subagent_%1_%2").arg(agentType).arg(++m_sequenceCounter);
    
    SubAgentInstance instance;
    instance.agentId = agentId;
    instance.type = agentType;
    instance.expertise = expertise.isEmpty() ? m_agentTypeConfigs[agentType].expertise : expertise;
    instance.createdAt = QDateTime::currentDateTime();
    instance.lastActivityAt = QDateTime::currentDateTime();
    instance.healthStatus.agentId = agentId;
    instance.healthStatus.isAlive = true;
    instance.healthStatus.isIdle = true;

    m_activeAgents[agentId] = instance;
    ++m_totalSpawned;

    emit agentSpawned(agentId);
    return agentId;
}

void SubAgentSystem::terminateSubAgent(const QString &agentId)
{
    if (!m_activeAgents.contains(agentId)) {
        return;
    }

    // Cancel any running tasks
    const auto &instance = m_activeAgents[agentId];
    for (const auto &taskId : instance.activeTasks) {
        cancelTask(taskId);
    }

    m_activeAgents.remove(agentId);
    emit agentTerminated(agentId);
}

void SubAgentSystem::terminateAllSubAgents()
{
    QStringList agentIds;
    for (const auto &agentId : m_activeAgents.keys()) {
        agentIds.append(agentId);
    }

    for (const auto &agentId : agentIds) {
        terminateSubAgent(agentId);
    }
}

QString SubAgentSystem::submitTask(const SubAgentTask &task, const QString &agentId)
{
    QString targetAgentId = agentId;
    
    // If no agent specified, select best available agent
    if (targetAgentId.isEmpty()) {
        targetAgentId = selectBestAgent(task);
        if (targetAgentId.isEmpty()) {
            qWarning() << "No available agents for task:" << task.taskId;
            return "";
        }
    }

    if (!m_activeAgents.contains(targetAgentId)) {
        qWarning() << "Agent not found:" << targetAgentId;
        return "";
    }

    // Store task and update agent
    m_pendingTasks[task.taskId] = task;
    m_activeAgents[targetAgentId].activeTasks.append(task.taskId);
    m_activeAgents[targetAgentId].healthStatus.isIdle = false;
    m_activeAgents[targetAgentId].lastActivityAt = QDateTime::currentDateTime();

    // Create and send message
    SubAgentMessage msg(targetAgentId, SubAgentMessageType::TaskRequest);
    msg.payload["taskId"] = task.taskId;
    msg.payload["type"] = task.type;
    msg.payload["parameters"] = task.parameters;
    msg.payload["timeoutMs"] = task.timeoutMs;

    emit taskDispatched(task.taskId, targetAgentId);
    emit messageReceived(msg);

    return task.taskId;
}

void SubAgentSystem::cancelTask(const QString &taskId)
{
    if (!m_pendingTasks.contains(taskId)) {
        return;
    }

    // Find and notify agent
    for (auto &instance : m_activeAgents) {
        if (instance.activeTasks.contains(taskId)) {
            SubAgentMessage msg(instance.agentId, SubAgentMessageType::TaskCancelled);
            msg.payload["taskId"] = taskId;
            emit messageReceived(msg);
            instance.activeTasks.removeAll(taskId);
            break;
        }
    }

    m_pendingTasks.remove(taskId);
    emit taskCancelled(taskId);
}

void SubAgentSystem::pauseTask(const QString &taskId)
{
    for (auto &instance : m_activeAgents) {
        if (instance.activeTasks.contains(taskId)) {
            SubAgentMessage msg(instance.agentId, SubAgentMessageType::PauseRequest);
            msg.payload["taskId"] = taskId;
            emit messageReceived(msg);
            break;
        }
    }
}

void SubAgentSystem::resumeTask(const QString &taskId)
{
    for (auto &instance : m_activeAgents) {
        if (instance.activeTasks.contains(taskId)) {
            SubAgentMessage msg(instance.agentId, SubAgentMessageType::ResumeRequest);
            msg.payload["taskId"] = taskId;
            emit messageReceived(msg);
            break;
        }
    }
}

SubAgentResult SubAgentSystem::waitForResult(const QString &taskId, int timeoutMs)
{
    // Wait for result with timeout
    QDateTime deadline = QDateTime::currentDateTime().addMSecs(timeoutMs);
    
    while (QDateTime::currentDateTime() < deadline) {
        if (m_completedResults.contains(taskId)) {
            return m_completedResults[taskId];
        }
        
        // Small sleep to avoid busy waiting
        QThread::msleep(100);
    }

    // Timeout - return error result
    SubAgentResult result;
    result.taskId = taskId;
    result.success = false;
    result.errorMessage = "Task timeout";
    return result;
}

SubAgentResult SubAgentSystem::getResult(const QString &taskId) const
{
    if (m_completedResults.contains(taskId)) {
        return m_completedResults[taskId];
    }
    
    SubAgentResult result;
    result.taskId = taskId;
    result.success = false;
    result.errorMessage = "Result not found";
    return result;
}

QVector<SubAgentResult> SubAgentSystem::collectResults(const QStringList &taskIds)
{
    QVector<SubAgentResult> results;
    for (const auto &taskId : taskIds) {
        results.append(getResult(taskId));
    }
    return results;
}

SubAgentProgress SubAgentSystem::getProgress(const QString &taskId) const
{
    if (m_taskProgress.contains(taskId)) {
        return m_taskProgress[taskId];
    }
    
    SubAgentProgress progress;
    progress.taskId = taskId;
    progress.percentComplete = 0;
    return progress;
}

int SubAgentSystem::getProgressPercent(const QString &taskId) const
{
    return getProgress(taskId).percentComplete;
}

SubAgentHealthStatus SubAgentSystem::getAgentHealth(const QString &agentId) const
{
    if (m_activeAgents.contains(agentId)) {
        return m_activeAgents[agentId].healthStatus;
    }
    
    SubAgentHealthStatus status;
    status.isAlive = false;
    return status;
}

QVector<SubAgentHealthStatus> SubAgentSystem::getAllAgentHealth() const
{
    QVector<SubAgentHealthStatus> statuses;
    for (const auto &instance : m_activeAgents) {
        statuses.append(instance.healthStatus);
    }
    return statuses;
}

void SubAgentSystem::healthCheck()
{
    for (auto it = m_activeAgents.begin(); it != m_activeAgents.end(); ++it) {
        auto &instance = it.value();
        
        // Check if agent should be considered unhealthy
        QDateTime now = QDateTime::currentDateTime();
        int timeSinceLastActivity = instance.lastActivityAt.msecsTo(now);
        
        if (timeSinceLastActivity > 300000) {  // 5 minutes
            instance.healthStatus.isAlive = false;
            emit agentHealthDegraded(instance.agentId, "No activity for 5 minutes");
        }
    }
}

SubAgentInstance SubAgentSystem::getAgentInstance(const QString &agentId) const
{
    if (m_activeAgents.contains(agentId)) {
        return m_activeAgents[agentId];
    }
    
    return SubAgentInstance();
}

QVector<SubAgentInstance> SubAgentSystem::getAllAgents() const
{
    QVector<SubAgentInstance> agents;
    for (const auto &instance : m_activeAgents) {
        agents.append(instance);
    }
    return agents;
}

QVector<SubAgentInstance> SubAgentSystem::getAgentsByType(const QString &type) const
{
    QVector<SubAgentInstance> agents;
    for (const auto &instance : m_activeAgents) {
        if (instance.type == type) {
            agents.append(instance);
        }
    }
    return agents;
}

int SubAgentSystem::getIdleAgentCount() const
{
    int count = 0;
    for (const auto &instance : m_activeAgents) {
        if (instance.healthStatus.isIdle) {
            ++count;
        }
    }
    return count;
}

int SubAgentSystem::getActiveAgentCount() const
{
    return m_activeAgents.size();
}

float SubAgentSystem::getAverageExecutionTimeMs() const
{
    if (m_completedResults.isEmpty()) {
        return 0.0f;
    }

    float sum = 0.0f;
    for (const auto &result : m_completedResults) {
        sum += result.executionTimeMs;
    }

    return sum / m_completedResults.size();
}

void SubAgentSystem::processMessage(const SubAgentMessage &message)
{
    switch (message.type) {
        case SubAgentMessageType::TaskCompleted:
            handleTaskCompleted(message);
            break;
        case SubAgentMessageType::TaskFailed:
            handleTaskFailed(message);
            break;
        case SubAgentMessageType::TaskProgress:
            handleTaskProgress(message);
            break;
        case SubAgentMessageType::HealthResponse:
            // Update agent health
            if (m_activeAgents.contains(message.agentId)) {
                m_activeAgents[message.agentId].lastActivityAt = QDateTime::currentDateTime();
            }
            break;
        default:
            break;
    }
}

void SubAgentSystem::handleTaskCompleted(const SubAgentMessage &message)
{
    QString taskId = message.payload["taskId"].toString();
    SubAgentResult result = SubAgentResult::fromJson(message.payload);
    
    m_completedResults[taskId] = result;
    updateAgentMetrics(message.agentId, result);

    // Remove from active tasks
    if (m_activeAgents.contains(message.agentId)) {
        auto &instance = m_activeAgents[message.agentId];
        instance.activeTasks.removeAll(taskId);
        instance.completedTasks.append(taskId);
        instance.healthStatus.totalTasksProcessed++;
        
        if (instance.activeTasks.isEmpty()) {
            instance.healthStatus.isIdle = true;
        }
    }

    m_pendingTasks.remove(taskId);
    emit taskCompleted(taskId, result);
}

void SubAgentSystem::handleTaskFailed(const SubAgentMessage &message)
{
    QString taskId = message.payload["taskId"].toString();
    QString error = message.payload["error"].toString();
    
    SubAgentResult result;
    result.taskId = taskId;
    result.success = false;
    result.errorMessage = error;
    
    m_completedResults[taskId] = result;

    if (m_activeAgents.contains(message.agentId)) {
        auto &instance = m_activeAgents[message.agentId];
        instance.activeTasks.removeAll(taskId);
        instance.completedTasks.append(taskId);
        instance.healthStatus.lastErrorMessage = error;
        
        if (instance.activeTasks.isEmpty()) {
            instance.healthStatus.isIdle = true;
        }
    }

    m_pendingTasks.remove(taskId);
    emit taskFailed(taskId, error);
}

void SubAgentSystem::handleTaskProgress(const SubAgentMessage &message)
{
    SubAgentProgress progress;
    progress.taskId = message.payload["taskId"].toString();
    progress.percentComplete = message.payload["percentComplete"].toInt();
    progress.currentStage = message.payload["currentStage"].toString();
    progress.statusMessage = message.payload["statusMessage"].toString();
    
    m_taskProgress[progress.taskId] = progress;
    emit taskProgress(progress.taskId, progress.percentComplete);
}

void SubAgentSystem::cleanupIdleAgents()
{
    QStringList toRemove;
    
    for (auto it = m_activeAgents.begin(); it != m_activeAgents.end(); ++it) {
        const auto &instance = it.value();
        
        if (instance.activeTasks.isEmpty()) {
            QDateTime now = QDateTime::currentDateTime();
            int idleTimeMs = instance.lastActivityAt.msecsTo(now);
            
            // Remove agents idle for more than 10 minutes
            if (idleTimeMs > 600000) {
                toRemove.append(instance.agentId);
            }
        }
    }

    for (const auto &agentId : toRemove) {
        terminateSubAgent(agentId);
    }
}

QString SubAgentSystem::selectBestAgent(const SubAgentTask &task)
{
    // Find idle agent of matching type
    QVector<SubAgentInstance> candidates;
    
    for (const auto &instance : m_activeAgents) {
        if (instance.healthStatus.isIdle && instance.healthStatus.isAlive) {
            candidates.append(instance);
        }
    }

    if (candidates.isEmpty()) {
        return "";  // No available agents
    }

    // Select agent with least completed tasks (load balance)
    return std::min_element(candidates.begin(), candidates.end(),
        [](const SubAgentInstance &a, const SubAgentInstance &b) {
            return a.completedTasks.size() < b.completedTasks.size();
        })->agentId;
}

void SubAgentSystem::updateAgentMetrics(const QString &agentId, const SubAgentResult &result)
{
    if (!m_activeAgents.contains(agentId)) {
        return;
    }

    auto &instance = m_activeAgents[agentId];
    instance.healthStatus.lastActivityAt = QDateTime::currentDateTime();
    
    if (!result.success) {
        instance.healthStatus.lastErrorMessage = result.errorMessage;
    }
}
