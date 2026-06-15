#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include "SubAgentMessage.h"

class SubAgentSystem;

/**
 * @class AgentScheduler
 * @brief Schedules and coordinates multi-agent task execution
 * 
 * Supports:
 * - Sequential task execution
 * - Parallel task execution with load balancing
 * - Task dependency resolution
 * - Result aggregation and voting
 * - Automatic failover and retry
 */

// ──────────────────────────────────────────────────────────────────────────────
// Execution Modes
// ──────────────────────────────────────────────────────────────────────────────

enum class ExecutionMode {
    Sequential,         // Execute tasks one by one in order
    Parallel,           // Execute all tasks concurrently
    BalancedParallel,   // Parallel but balanced across agent pool
    DependencyGraph,    // Execute based on dependency graph
    Adaptive            // Automatically choose based on task properties
};

enum class ResultAggregationMode {
    All,                // Collect all results
    FirstSuccess,       // Stop at first successful result
    Majority,           // Use majority vote
    Weighted,           // Use weighted scoring
    Consensus           // Require consensus from all agents
};

// ──────────────────────────────────────────────────────────────────────────────
// Scheduling Configuration
// ──────────────────────────────────────────────────────────────────────────────

struct ScheduleConfig {
    ExecutionMode executionMode{ExecutionMode::Parallel};
    ResultAggregationMode aggregationMode{ResultAggregationMode::All};
    
    int maxConcurrentAgents{5};
    int maxRetries{2};
    int timeoutPerTaskMs{30000};
    int globalTimeoutMs{120000};
    
    bool enableFallback{true};          // Fallback to sequential if parallel fails
    bool enableMetrics{true};           // Track performance metrics
    bool enableLogging{true};           // Log all scheduling events
};

// ──────────────────────────────────────────────────────────────────────────────
// Schedule Result
// ──────────────────────────────────────────────────────────────────────────────

struct ScheduleResult {
    bool success{false};
    QString error;
    
    QMap<QString, SubAgentResult> taskResults;      // taskId → result
    QMap<QString, QString> taskToAgent;             // taskId → agentId
    ConsensusResult consensus;                      // For voting scenarios
    
    int totalTasksScheduled{0};
    int successfulTasks{0};
    int failedTasks{0};
    
    int totalExecutionTimeMs{0};
    float averageTaskTimeMs{0.0f};
    
    QJsonObject performanceMetrics;
};

// ──────────────────────────────────────────────────────────────────────────────
// AgentScheduler
// ──────────────────────────────────────────────────────────────────────────────

class AgentScheduler : public QObject {
    Q_OBJECT

public:
    explicit AgentScheduler(QObject *parent = nullptr);
    ~AgentScheduler();

    void setSubAgentSystem(SubAgentSystem *system);
    void setConfig(const ScheduleConfig &config);

    // Single task scheduling
    ScheduleResult scheduleSingleTask(const SubAgentTask &task);

    // Multiple tasks scheduling
    ScheduleResult scheduleMultipleTasks(const QVector<SubAgentTask> &tasks);

    // Dependency-aware scheduling
    ScheduleResult scheduleWithDependencies(
        const QVector<SubAgentTask> &tasks,
        const QMap<QString, QStringList> &dependencies);

    // Parallel execution with multiple agents voting
    ScheduleResult scheduleWithVoting(
        const SubAgentTask &task,
        const QStringList &agentIds,
        int minVotesForConsensus = -1);  // -1 means all agents must vote

    // Query scheduling status
    bool isScheduling() const { return m_isScheduling; }
    ScheduleResult getLastResult() const { return m_lastResult; }

signals:
    void schedulingStarted(int taskCount);
    void taskScheduled(const QString &taskId, const QString &agentId);
    void taskResultReceived(const QString &taskId, bool success);
    void schedulingCompleted(const ScheduleResult &result);
    void schedulingError(const QString &error);

private:
    ScheduleResult executeSequential(const QVector<SubAgentTask> &tasks);
    ScheduleResult executeParallel(const QVector<SubAgentTask> &tasks);
    ScheduleResult executeBalancedParallel(const QVector<SubAgentTask> &tasks);
    ScheduleResult executeDependencyGraph(
        const QVector<SubAgentTask> &tasks,
        const QMap<QString, QStringList> &dependencies);
    
    QVector<SubAgentTask> resolveDependencies(
        const QVector<SubAgentTask> &tasks,
        const QMap<QString, QStringList> &dependencies);
    
    ConsensusResult aggregateVotes(const QVector<SubAgentResult> &results);
    SubAgentResult selectBestResult(const QVector<SubAgentResult> &results);
    
    void handleTaskCompletion(const QString &taskId, const SubAgentResult &result);
    void handleSchedulingTimeout();
    
    bool validateDependencies(
        const QVector<SubAgentTask> &tasks,
        const QMap<QString, QStringList> &dependencies) const;

    // Member variables
    SubAgentSystem *m_subAgentSystem{nullptr};
    ScheduleConfig m_config;
    
    bool m_isScheduling{false};
    ScheduleResult m_lastResult;
    
    QMap<QString, SubAgentResult> m_pendingResults;
    int m_completedCount{0};
    int m_totalToSchedule{0};
};
