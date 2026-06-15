#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <functional>
#include "SubAgentMessage.h"

class SubAgentSystem;
class AgentScheduler;
class BackgroundAgentManager;

/**
 * @class AgentCoordinator
 * @brief High-level coordination of multiple agents
 * 
 * Orchestrates complex multi-agent workflows including:
 * - Sequential task pipelines
 * - Parallel voting and consensus
 * - Error handling and recovery
 * - Progress aggregation
 */

// ──────────────────────────────────────────────────────────────────────────────
// Coordination Strategies
// ──────────────────────────────────────────────────────────────────────────────

enum class CoordinationStrategy {
    Sequential,         // One task after another
    ParallelVoting,     // Multiple agents vote on result
    ParallelPipeline,   // Parallel tasks with inter-dependencies
    Hierarchical,       // Parent-child agent structure
    Custom              // Custom coordination logic
};

// ──────────────────────────────────────────────────────────────────────────────
// Coordination Result
// ──────────────────────────────────────────────────────────────────────────────

struct CoordinationResult {
    bool success{false};
    QString error;
    
    // Multi-agent results
    QMap<QString, SubAgentResult> agentResults;      // agentId → result
    QVector<ConsensusResult> consensusResults;       // For voting
    
    // Timing
    int totalTimeMs{0};
    QMap<QString, int> agentExecutionTimes;
    
    // Quality metrics
    float overallQualityScore{0.0f};
    QMap<QString, float> agentQualityScores;
};

// ──────────────────────────────────────────────────────────────────────────────
// Workflow Definition
// ──────────────────────────────────────────────────────────────────────────────

struct AgentWorkflowStep {
    QString stepId;
    QString description;
    
    QStringList agentIds;               // Agents to use for this step
    SubAgentTask task;
    
    CoordinationStrategy strategy{CoordinationStrategy::ParallelVoting};
    
    QStringList dependsOn;              // Step IDs this depends on
    
    int retryCount{0};
    int maxRetries{2};
    
    bool success{false};
    SubAgentResult result;
};

// ──────────────────────────────────────────────────────────────────────────────
// AgentCoordinator
// ──────────────────────────────────────────────────────────────────────────────

class AgentCoordinator : public QObject {
    Q_OBJECT

public:
    explicit AgentCoordinator(QObject *parent = nullptr);
    ~AgentCoordinator();

    // Setup
    void setSubAgentSystem(SubAgentSystem *system);
    void setAgentScheduler(AgentScheduler *scheduler);
    void setBackgroundManager(BackgroundAgentManager *manager);

    // Simple multi-agent execution
    CoordinationResult executeWithMultipleAgents(
        const SubAgentTask &task,
        const QStringList &agentIds,
        CoordinationStrategy strategy = CoordinationStrategy::ParallelVoting);

    // Workflow execution
    QString submitWorkflow(const QVector<AgentWorkflowStep> &steps);
    CoordinationResult waitForWorkflow(const QString &workflowId, int timeoutMs = -1);
    void cancelWorkflow(const QString &workflowId);

    // Agent selection and validation
    QStringList selectAgentsForTask(const SubAgentTask &task, int count = -1);
    bool validateAgentsForTask(const QStringList &agentIds, const SubAgentTask &task) const;

    // Monitoring
    CoordinationResult getWorkflowResult(const QString &workflowId) const;
    float getWorkflowProgress(const QString &workflowId) const;

signals:
    void coordinationStarted(int agentCount);
    void agentStarted(const QString &agentId);
    void agentCompleted(const QString &agentId, bool success);
    void stepCompleted(const QString &stepId);
    void workflowCompleted(const QString &workflowId);
    void coordinationError(const QString &error);

private:
    CoordinationResult executeParallelVoting(
        const SubAgentTask &task,
        const QStringList &agentIds);

    CoordinationResult executeSequential(
        const SubAgentTask &task,
        const QStringList &agentIds);

    CoordinationResult executeParallelPipeline(
        const QVector<AgentWorkflowStep> &steps);

    void executeWorkflowStep(const AgentWorkflowStep &step);
    void handleStepCompletion(const QString &stepId, const SubAgentResult &result);

    ConsensusResult buildConsensus(const QVector<SubAgentResult> &results);
    QStringList sortAgentsByRanking(const QStringList &agentIds) const;

    // Member variables
    SubAgentSystem *m_subAgentSystem{nullptr};
    AgentScheduler *m_agentScheduler{nullptr};
    BackgroundAgentManager *m_backgroundManager{nullptr};

    QMap<QString, QVector<AgentWorkflowStep>> m_workflows;
    QMap<QString, CoordinationResult> m_workflowResults;
    QMap<QString, float> m_workflowProgress;

    int m_sequenceCounter{0};
};
