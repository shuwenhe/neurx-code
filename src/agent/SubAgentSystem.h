#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QMap>
#include <memory>
#include "SubAgentMessage.h"
#include "agent/AgentMessage.h"

class AgentEngine;
class LLMProvider;

/**
 * @class SubAgentSystem
 * @brief Spawns, manages, and coordinates sub-agents
 * 
 * Features:
 * - Dynamic sub-agent creation from agent pool
 * - Task distribution and tracking
 * - Result collection and aggregation
 * - Agent health monitoring
 * - Automatic cleanup and resource management
 */

// ──────────────────────────────────────────────────────────────────────────────
// SubAgent Configuration
// ──────────────────────────────────────────────────────────────────────────────

struct SubAgentConfig {
    QString agentType;                  // Type: "reviewer", "architect", "tester", etc.
    QString expertise;                  // Specialization
    int maxConcurrentTasks{1};
    int maxMemoryMb{512};
    QString systemPrompt;
    QJsonObject preferences;
    bool autoCleanup{true};             // Auto-cleanup when task done
};

struct SubAgentInstance {
    QString agentId;                    // Unique instance ID
    QString type;
    QString expertise;
    SubAgentHealthStatus healthStatus;
    
    QVector<QString> activeTasks;       // Currently running task IDs
    QVector<QString> completedTasks;    // Finished task IDs
    
    QDateTime createdAt;
    QDateTime lastActivityAt;
    
    bool isValid() const { return !agentId.isEmpty(); }
};

// ──────────────────────────────────────────────────────────────────────────────
// SubAgentSystem
// ──────────────────────────────────────────────────────────────────────────────

class SubAgentSystem : public QObject {
    Q_OBJECT

public:
    explicit SubAgentSystem(QObject *parent = nullptr);
    ~SubAgentSystem();

    // Configuration
    void setLLMProvider(LLMProvider *provider);
    void setParentAgentEngine(AgentEngine *parent);
    void registerAgentType(const SubAgentConfig &config);

    // Sub-agent lifecycle
    QString spawnSubAgent(const QString &agentType, const QString &expertise = "");
    void terminateSubAgent(const QString &agentId);
    void terminateAllSubAgents();

    // Task dispatch
    QString submitTask(const SubAgentTask &task, const QString &agentId = "");
    void cancelTask(const QString &taskId);
    void pauseTask(const QString &taskId);
    void resumeTask(const QString &taskId);

    // Result collection
    SubAgentResult waitForResult(const QString &taskId, int timeoutMs = 30000);
    SubAgentResult getResult(const QString &taskId) const;
    QVector<SubAgentResult> collectResults(const QStringList &taskIds);

    // Progress tracking
    SubAgentProgress getProgress(const QString &taskId) const;
    int getProgressPercent(const QString &taskId) const;

    // Agent health monitoring
    SubAgentHealthStatus getAgentHealth(const QString &agentId) const;
    QVector<SubAgentHealthStatus> getAllAgentHealth() const;
    void healthCheck();

    // Agent querying
    SubAgentInstance getAgentInstance(const QString &agentId) const;
    QVector<SubAgentInstance> getAllAgents() const;
    QVector<SubAgentInstance> getAgentsByType(const QString &type) const;
    int getIdleAgentCount() const;

    // Statistics
    int getTotalSpawnedAgents() const { return m_totalSpawned; }
    int getActiveAgentCount() const;
    float getAverageExecutionTimeMs() const;

signals:
    // Agent lifecycle
    void agentSpawned(const QString &agentId);
    void agentTerminated(const QString &agentId);

    // Task events
    void taskDispatched(const QString &taskId, const QString &agentId);
    void taskStarted(const QString &taskId);
    void taskProgress(const QString &taskId, int percentComplete);
    void taskCompleted(const QString &taskId, const SubAgentResult &result);
    void taskFailed(const QString &taskId, const QString &error);
    void taskCancelled(const QString &taskId);

    // Agent events
    void agentHealthDegraded(const QString &agentId, const QString &reason);
    void agentErrorOccurred(const QString &agentId, const QString &error);

    // Message events (for streaming)
    void messageReceived(const SubAgentMessage &message);

private:
    void processMessage(const SubAgentMessage &message);
    void handleTaskCompleted(const SubAgentMessage &message);
    void handleTaskFailed(const SubAgentMessage &message);
    void handleTaskProgress(const SubAgentMessage &message);
    void cleanupIdleAgents();
    QString selectBestAgent(const SubAgentTask &task);
    void updateAgentMetrics(const QString &agentId, const SubAgentResult &result);

    // Member variables
    LLMProvider *m_provider{nullptr};
    AgentEngine *m_parentEngine{nullptr};

    QMap<QString, SubAgentConfig> m_agentTypeConfigs;
    QMap<QString, SubAgentInstance> m_activeAgents;
    QMap<QString, SubAgentTask> m_pendingTasks;
    QMap<QString, SubAgentResult> m_completedResults;
    QMap<QString, SubAgentProgress> m_taskProgress;

    int m_totalSpawned{0};
    int m_sequenceCounter{0};
};
