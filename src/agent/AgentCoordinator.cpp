#include "AgentCoordinator.h"
#include "SubAgentSystem.h"
#include "AgentScheduler.h"
#include "BackgroundAgentManager.h"
#include <QDateTime>
#include <QUuid>
#include <algorithm>

AgentCoordinator::AgentCoordinator(QObject *parent)
    : QObject(parent)
{
}

AgentCoordinator::~AgentCoordinator()
{
}

void AgentCoordinator::setSubAgentSystem(SubAgentSystem *system)
{
    m_subAgentSystem = system;
}

void AgentCoordinator::setAgentScheduler(AgentScheduler *scheduler)
{
    m_agentScheduler = scheduler;
}

void AgentCoordinator::setBackgroundManager(BackgroundAgentManager *manager)
{
    m_backgroundManager = manager;
}

CoordinationResult AgentCoordinator::executeWithMultipleAgents(
    const SubAgentTask &task,
    const QStringList &agentIds,
    CoordinationStrategy strategy)
{
    if (!m_subAgentSystem || agentIds.isEmpty()) {
        CoordinationResult result;
        result.success = false;
        result.error = "SubAgentSystem not set or no agents provided";
        return result;
    }

    emit coordinationStarted(agentIds.size());

    switch (strategy) {
        case CoordinationStrategy::ParallelVoting:
            return executeParallelVoting(task, agentIds);
        case CoordinationStrategy::Sequential:
            return executeSequential(task, agentIds);
        default:
            return executeParallelVoting(task, agentIds);
    }
}

QString AgentCoordinator::submitWorkflow(const QVector<AgentWorkflowStep> &steps)
{
    QString workflowId = QString("workflow_%1").arg(++m_sequenceCounter);
    m_workflows[workflowId] = steps;
    m_workflowProgress[workflowId] = 0.0f;

    // Execute workflow asynchronously
    QMetaObject::invokeMethod(this, [this, workflowId]() {
        executeParallelPipeline(m_workflows[workflowId]);
    }, Qt::QueuedConnection);

    return workflowId;
}

CoordinationResult AgentCoordinator::waitForWorkflow(const QString &workflowId, int timeoutMs)
{
    QDateTime deadline;
    if (timeoutMs > 0) {
        deadline = QDateTime::currentDateTime().addMSecs(timeoutMs);
    }

    while (true) {
        if (m_workflowResults.contains(workflowId)) {
            return m_workflowResults[workflowId];
        }

        if (deadline.isValid() && QDateTime::currentDateTime() >= deadline) {
            CoordinationResult result;
            result.success = false;
            result.error = "Workflow timeout";
            return result;
        }

        QThread::msleep(100);
    }
}

void AgentCoordinator::cancelWorkflow(const QString &workflowId)
{
    if (m_workflows.contains(workflowId)) {
        m_workflows.remove(workflowId);
    }
}

QStringList AgentCoordinator::selectAgentsForTask(const SubAgentTask &task, int count)
{
    if (!m_subAgentSystem) {
        return QStringList();
    }

    auto agents = m_subAgentSystem->getAllAgents();
    
    if (count <= 0) {
        count = std::min(3, static_cast<int>(agents.size()));  // Default to 3 agents
    }

    QStringList selectedIds;
    for (int i = 0; i < std::min(count, static_cast<int>(agents.size())); ++i) {
        selectedIds.append(agents[i].agentId);
    }

    return selectedIds;
}

bool AgentCoordinator::validateAgentsForTask(const QStringList &agentIds, const SubAgentTask &task) const
{
    if (!m_subAgentSystem) {
        return false;
    }

    for (const auto &agentId : agentIds) {
        auto agent = m_subAgentSystem->getAgentInstance(agentId);
        if (!agent.isValid() || !agent.healthStatus.isAlive) {
            return false;
        }
    }

    return true;
}

CoordinationResult AgentCoordinator::getWorkflowResult(const QString &workflowId) const
{
    if (m_workflowResults.contains(workflowId)) {
        return m_workflowResults[workflowId];
    }

    CoordinationResult result;
    result.success = false;
    result.error = "Workflow not found";
    return result;
}

float AgentCoordinator::getWorkflowProgress(const QString &workflowId) const
{
    if (m_workflowProgress.contains(workflowId)) {
        return m_workflowProgress[workflowId];
    }
    return 0.0f;
}

CoordinationResult AgentCoordinator::executeParallelVoting(
    const SubAgentTask &task,
    const QStringList &agentIds)
{
    CoordinationResult result;
    QDateTime startTime = QDateTime::currentDateTime();

    QVector<SubAgentResult> agentResults;

    // Submit task to each agent
    for (const auto &agentId : agentIds) {
        m_subAgentSystem->submitTask(task, agentId);
        emit agentStarted(agentId);
    }

    // Collect results from all agents
    for (const auto &agentId : agentIds) {
        SubAgentResult agentResult = m_subAgentSystem->waitForResult(task.taskId);
        agentResults.append(agentResult);
        result.agentResults[agentId] = agentResult;

        emit agentCompleted(agentId, agentResult.success);
    }

    // Build consensus
    result.consensusResults.append(buildConsensus(agentResults));
    
    result.success = !result.consensusResults.isEmpty();
    result.totalTimeMs = startTime.msecsTo(QDateTime::currentDateTime());

    // Calculate overall quality
    if (!agentResults.isEmpty()) {
        float totalScore = 0.0f;
        for (const auto &res : agentResults) {
            totalScore += res.qualityScore;
        }
        result.overallQualityScore = totalScore / agentResults.size();
    }

    return result;
}

CoordinationResult AgentCoordinator::executeSequential(
    const SubAgentTask &task,
    const QStringList &agentIds)
{
    CoordinationResult result;
    QDateTime startTime = QDateTime::currentDateTime();

    for (const auto &agentId : agentIds) {
        emit agentStarted(agentId);

        m_subAgentSystem->submitTask(task, agentId);
        SubAgentResult agentResult = m_subAgentSystem->waitForResult(task.taskId);
        
        result.agentResults[agentId] = agentResult;
        emit agentCompleted(agentId, agentResult.success);

        // Stop at first success if preferred
        if (agentResult.success) {
            result.success = true;
            break;
        }
    }

    result.totalTimeMs = startTime.msecsTo(QDateTime::currentDateTime());
    return result;
}

CoordinationResult AgentCoordinator::executeParallelPipeline(
    const QVector<AgentWorkflowStep> &steps)
{
    CoordinationResult result;
    QDateTime workflowStartTime = QDateTime::currentDateTime();

    QMap<QString, bool> stepFlags;
    for (const auto &step : steps) {
        stepFlags[step.stepId] = false;
    }

    // Execute steps respecting dependencies
    bool progress = true;
    while (progress) {
        progress = false;

        for (const auto &step : steps) {
            if (stepFlags[step.stepId]) {
                continue;
            }

            // Check dependencies
            bool depsMet = true;
            for (const auto &depId : step.dependsOn) {
                if (!stepFlags[depId]) {
                    depsMet = false;
                    break;
                }
            }

            if (depsMet) {
                executeWorkflowStep(step);
                stepFlags[step.stepId] = true;
                progress = true;

                emit stepCompleted(step.stepId);

                // Update progress
                int completedSteps = 0;
                for (const auto &completed : stepFlags) {
                    if (completed) ++completedSteps;
                }
                m_workflowProgress[QString("workflow")] = 
                    static_cast<float>(completedSteps) / steps.size() * 100.0f;
            }
        }
    }

    result.success = true;
    result.totalTimeMs = workflowStartTime.msecsTo(QDateTime::currentDateTime());

    // Find first workflow result
    QString workflowId;
    for (auto it = m_workflowResults.begin(); it != m_workflowResults.end(); ++it) {
        workflowId = it.key();
        result = it.value();
        break;
    }

    if (!workflowId.isEmpty()) {
        emit workflowCompleted(workflowId);
    }

    return result;
}

void AgentCoordinator::executeWorkflowStep(const AgentWorkflowStep &step)
{
    if (step.agentIds.isEmpty()) {
        return;
    }

    // Execute with the specified agents
    CoordinationResult stepResult = executeWithMultipleAgents(
        step.task,
        step.agentIds,
        step.strategy);
}

void AgentCoordinator::handleStepCompletion(const QString &stepId, const SubAgentResult &result)
{
    // Update workflow results
    for (auto it = m_workflows.begin(); it != m_workflows.end(); ++it) {
        for (auto &step : it.value()) {
            if (step.stepId == stepId) {
                step.success = result.success;
                step.result = result;
                return;
            }
        }
    }
}

ConsensusResult AgentCoordinator::buildConsensus(const QVector<SubAgentResult> &results)
{
    ConsensusResult consensus;

    // Count decisions
    QMap<QString, int> decisions;
    for (const auto &result : results) {
        QString decision = result.data["decision"].toString("approved");
        decisions[decision]++;
    }

    // Find majority
    int maxVotes = 0;
    QString majorityDecision = "no_consensus";
    for (auto it = decisions.begin(); it != decisions.end(); ++it) {
        if (it.value() > maxVotes) {
            maxVotes = it.value();
            majorityDecision = it.key();
        }
    }

    consensus.finalDecision = majorityDecision;
    consensus.consensusStrength = static_cast<float>(maxVotes) / std::max(1, static_cast<int>(results.size()));

    return consensus;
}

QStringList AgentCoordinator::sortAgentsByRanking(const QStringList &agentIds) const
{
    // Sort agents by their performance metrics
    QStringList sorted = agentIds;
    
    std::sort(sorted.begin(), sorted.end(),
        [this](const QString &a, const QString &b) {
            if (!m_subAgentSystem) return false;
            
            auto healthA = m_subAgentSystem->getAgentHealth(a);
            auto healthB = m_subAgentSystem->getAgentHealth(b);
            
            return healthA.totalTasksProcessed > healthB.totalTasksProcessed;
        });

    return sorted;
}
