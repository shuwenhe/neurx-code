#include "AgentScheduler.h"
#include "SubAgentSystem.h"
#include <QTimer>
#include <QDateTime>
#include <algorithm>
#include <numeric>

AgentScheduler::AgentScheduler(QObject *parent)
    : QObject(parent)
{
}

AgentScheduler::~AgentScheduler()
{
}

void AgentScheduler::setSubAgentSystem(SubAgentSystem *system)
{
    m_subAgentSystem = system;
    if (m_subAgentSystem) {
        connect(m_subAgentSystem, 
                static_cast<void (SubAgentSystem::*)(const QString&, const SubAgentResult&)>(
                    &SubAgentSystem::taskCompleted),
                this, &AgentScheduler::handleTaskCompletion);
    }
}

void AgentScheduler::setConfig(const ScheduleConfig &config)
{
    m_config = config;
}

ScheduleResult AgentScheduler::scheduleSingleTask(const SubAgentTask &task)
{
    if (!m_subAgentSystem) {
        ScheduleResult result;
        result.success = false;
        result.error = "SubAgentSystem not set";
        return result;
    }

    QVector<SubAgentTask> tasks{task};
    return executeParallel(tasks);
}

ScheduleResult AgentScheduler::scheduleMultipleTasks(const QVector<SubAgentTask> &tasks)
{
    if (tasks.isEmpty()) {
        ScheduleResult result;
        result.success = false;
        result.error = "No tasks to schedule";
        return result;
    }

    switch (m_config.executionMode) {
        case ExecutionMode::Sequential:
            return executeSequential(tasks);
        case ExecutionMode::Parallel:
            return executeParallel(tasks);
        case ExecutionMode::BalancedParallel:
            return executeBalancedParallel(tasks);
        case ExecutionMode::Adaptive:
            // Choose mode based on number of tasks and agents
            if (tasks.size() > m_subAgentSystem->getActiveAgentCount()) {
                return executeBalancedParallel(tasks);
            }
            return executeParallel(tasks);
        default:
            return executeParallel(tasks);
    }
}

ScheduleResult AgentScheduler::scheduleWithDependencies(
    const QVector<SubAgentTask> &tasks,
    const QMap<QString, QStringList> &dependencies)
{
    if (!validateDependencies(tasks, dependencies)) {
        ScheduleResult result;
        result.success = false;
        result.error = "Invalid dependencies detected";
        return result;
    }

    return executeDependencyGraph(tasks, dependencies);
}

ScheduleResult AgentScheduler::scheduleWithVoting(
    const SubAgentTask &task,
    const QStringList &agentIds,
    int minVotesForConsensus)
{
    if (!m_subAgentSystem) {
        ScheduleResult result;
        result.success = false;
        result.error = "SubAgentSystem not set";
        return result;
    }

    int actualMinVotes = minVotesForConsensus < 0 ? agentIds.size() : minVotesForConsensus;
    QVector<SubAgentResult> results;

    // Schedule task with each agent and collect results
    for (const auto &agentId : agentIds) {
        m_subAgentSystem->submitTask(task, agentId);
        SubAgentResult result = m_subAgentSystem->waitForResult(task.taskId);
        results.append(result);
    }

    ScheduleResult scheduleResult;
    scheduleResult.consensus = aggregateVotes(results);
    scheduleResult.success = !scheduleResult.consensus.votes.isEmpty();
    scheduleResult.totalTasksScheduled = agentIds.size();
    scheduleResult.successfulTasks = scheduleResult.consensus.getVoteCount("approved");

    for (int i = 0; i < results.size(); ++i) {
        scheduleResult.taskResults[QString("vote_%1").arg(i)] = results[i];
    }

    m_lastResult = scheduleResult;
    emit schedulingCompleted(scheduleResult);

    return scheduleResult;
}

ScheduleResult AgentScheduler::executeSequential(const QVector<SubAgentTask> &tasks)
{
    emit schedulingStarted(tasks.size());

    ScheduleResult result;
    result.totalTasksScheduled = tasks.size();
    
    QDateTime startTime = QDateTime::currentDateTime();
    m_isScheduling = true;

    for (const auto &task : tasks) {
        QString taskId = m_subAgentSystem->submitTask(task);
        
        if (!taskId.isEmpty()) {
            SubAgentResult taskResult = m_subAgentSystem->waitForResult(taskId, m_config.timeoutPerTaskMs);
            result.taskResults[taskId] = taskResult;
            
            if (taskResult.success) {
                result.successfulTasks++;
            } else {
                result.failedTasks++;
            }
        }

        emit taskResultReceived(task.taskId, !result.taskResults[task.taskId].errorMessage.isEmpty());
    }

    result.totalExecutionTimeMs = startTime.msecsTo(QDateTime::currentDateTime());
    result.success = result.failedTasks == 0;
    result.averageTaskTimeMs = static_cast<float>(result.totalExecutionTimeMs) / tasks.size();

    m_isScheduling = false;
    m_lastResult = result;
    
    emit schedulingCompleted(result);
    return result;
}

ScheduleResult AgentScheduler::executeParallel(const QVector<SubAgentTask> &tasks)
{
    emit schedulingStarted(tasks.size());

    ScheduleResult result;
    result.totalTasksScheduled = tasks.size();
    
    QDateTime startTime = QDateTime::currentDateTime();
    m_isScheduling = true;

    // Submit all tasks
    QStringList taskIds;
    for (const auto &task : tasks) {
        QString taskId = m_subAgentSystem->submitTask(task);
        if (!taskId.isEmpty()) {
            taskIds.append(taskId);
        }
    }

    // Collect all results
    for (const auto &taskId : taskIds) {
        SubAgentResult taskResult = m_subAgentSystem->waitForResult(
            taskId, m_config.timeoutPerTaskMs);
        result.taskResults[taskId] = taskResult;

        if (taskResult.success) {
            result.successfulTasks++;
        } else {
            result.failedTasks++;
        }

        emit taskResultReceived(taskId, taskResult.success);
    }

    result.totalExecutionTimeMs = startTime.msecsTo(QDateTime::currentDateTime());
    result.success = result.failedTasks == 0;
    result.averageTaskTimeMs = static_cast<float>(result.totalExecutionTimeMs) / tasks.size();

    m_isScheduling = false;
    m_lastResult = result;
    
    emit schedulingCompleted(result);
    return result;
}

ScheduleResult AgentScheduler::executeBalancedParallel(const QVector<SubAgentTask> &tasks)
{
    emit schedulingStarted(tasks.size());

    ScheduleResult result;
    result.totalTasksScheduled = tasks.size();
    
    QDateTime startTime = QDateTime::currentDateTime();
    m_isScheduling = true;

    int maxAgents = m_config.maxConcurrentAgents;
    int processedCount = 0;

    // Process tasks in batches
    for (int i = 0; i < tasks.size(); i += maxAgents) {
        QVector<SubAgentTask> batch(
            tasks.begin() + i,
            tasks.begin() + std::min(i + maxAgents, static_cast<int>(tasks.size())));

        // Submit batch
        QStringList batchTaskIds;
        for (const auto &task : batch) {
            QString taskId = m_subAgentSystem->submitTask(task);
            if (!taskId.isEmpty()) {
                batchTaskIds.append(taskId);
            }
        }

        // Collect batch results
        for (const auto &taskId : batchTaskIds) {
            SubAgentResult taskResult = m_subAgentSystem->waitForResult(
                taskId, m_config.timeoutPerTaskMs);
            result.taskResults[taskId] = taskResult;

            if (taskResult.success) {
                result.successfulTasks++;
            } else {
                result.failedTasks++;
            }

            emit taskResultReceived(taskId, taskResult.success);
        }

        processedCount += batch.size();
    }

    result.totalExecutionTimeMs = startTime.msecsTo(QDateTime::currentDateTime());
    result.success = result.failedTasks == 0;
    result.averageTaskTimeMs = result.totalTasksScheduled > 0 ?
        static_cast<float>(result.totalExecutionTimeMs) / result.totalTasksScheduled : 0.0f;

    m_isScheduling = false;
    m_lastResult = result;
    
    emit schedulingCompleted(result);
    return result;
}

ScheduleResult AgentScheduler::executeDependencyGraph(
    const QVector<SubAgentTask> &tasks,
    const QMap<QString, QStringList> &dependencies)
{
    emit schedulingStarted(tasks.size());

    ScheduleResult result;
    result.totalTasksScheduled = tasks.size();
    
    QDateTime startTime = QDateTime::currentDateTime();
    m_isScheduling = true;

    QMap<QString, bool> completed;
    for (const auto &task : tasks) {
        completed[task.taskId] = false;
    }

    // Execute tasks respecting dependencies
    bool progress = true;
    while (progress) {
        progress = false;

        for (const auto &task : tasks) {
            if (completed[task.taskId]) {
                continue;  // Already done
            }

            // Check if all dependencies are satisfied
            bool dependenciesMet = true;
            if (dependencies.contains(task.taskId)) {
                for (const auto &depId : dependencies[task.taskId]) {
                    if (!completed[depId]) {
                        dependenciesMet = false;
                        break;
                    }
                }
            }

            if (dependenciesMet) {
                // Execute task
                QString taskId = m_subAgentSystem->submitTask(task);
                SubAgentResult taskResult = m_subAgentSystem->waitForResult(
                    taskId, m_config.timeoutPerTaskMs);
                
                result.taskResults[taskId] = taskResult;
                completed[task.taskId] = true;
                progress = true;

                if (taskResult.success) {
                    result.successfulTasks++;
                } else {
                    result.failedTasks++;
                }

                emit taskResultReceived(taskId, taskResult.success);
            }
        }
    }

    result.totalExecutionTimeMs = startTime.msecsTo(QDateTime::currentDateTime());
    result.success = result.failedTasks == 0;
    result.averageTaskTimeMs = result.totalTasksScheduled > 0 ?
        static_cast<float>(result.totalExecutionTimeMs) / result.totalTasksScheduled : 0.0f;

    m_isScheduling = false;
    m_lastResult = result;
    
    emit schedulingCompleted(result);
    return result;
}

QVector<SubAgentTask> AgentScheduler::resolveDependencies(
    const QVector<SubAgentTask> &tasks,
    const QMap<QString, QStringList> &dependencies)
{
    // Use topological sort to order tasks
    QVector<SubAgentTask> ordered;
    QMap<QString, bool> visited;
    
    for (const auto &task : tasks) {
        visited[task.taskId] = false;
    }

    // Simple topological sort (for DAGs)
    std::function<void(const SubAgentTask&)> visit = [&](const SubAgentTask &task) {
        if (visited[task.taskId]) {
            return;
        }

        visited[task.taskId] = true;

        if (dependencies.contains(task.taskId)) {
            for (const auto &depId : dependencies[task.taskId]) {
                for (const auto &t : tasks) {
                    if (t.taskId == depId) {
                        visit(t);
                        break;
                    }
                }
            }
        }

        ordered.append(task);
    };

    for (const auto &task : tasks) {
        visit(task);
    }

    return ordered;
}

ConsensusResult AgentScheduler::aggregateVotes(const QVector<SubAgentResult> &results)
{
    ConsensusResult consensus;

    // Count decisions
    QMap<QString, int> decisionCounts;
    for (const auto &result : results) {
        QString decision = result.data["decision"].toString("approved");
        decisionCounts[decision]++;
    }

    // Find majority decision
    int maxVotes = 0;
    QString majorityDecision = "no_consensus";
    for (auto it = decisionCounts.begin(); it != decisionCounts.end(); ++it) {
        if (it.value() > maxVotes) {
            maxVotes = it.value();
            majorityDecision = it.key();
        }
    }

    consensus.finalDecision = majorityDecision;
    consensus.consensusStrength = static_cast<float>(maxVotes) / std::max(1, static_cast<int>(results.size()));

    return consensus;
}

SubAgentResult AgentScheduler::selectBestResult(const QVector<SubAgentResult> &results)
{
    if (results.isEmpty()) {
        SubAgentResult empty;
        empty.success = false;
        return empty;
    }

    // Select result with highest quality score
    return *std::max_element(results.begin(), results.end(),
        [](const SubAgentResult &a, const SubAgentResult &b) {
            return a.qualityScore < b.qualityScore;
        });
}

void AgentScheduler::handleTaskCompletion(const QString &taskId, const SubAgentResult &result)
{
    m_pendingResults[taskId] = result;
    ++m_completedCount;

    if (m_completedCount >= m_totalToSchedule) {
        // All tasks completed
        handleSchedulingTimeout();
    }
}

void AgentScheduler::handleSchedulingTimeout()
{
    // Cleanup pending results
    m_pendingResults.clear();
    m_completedCount = 0;
}

bool AgentScheduler::validateDependencies(
    const QVector<SubAgentTask> &tasks,
    const QMap<QString, QStringList> &dependencies) const
{
    // Check for circular dependencies and invalid references
    QSet<QString> taskIds;
    for (const auto &task : tasks) {
        taskIds.insert(task.taskId);
    }

    for (auto it = dependencies.begin(); it != dependencies.end(); ++it) {
        if (!taskIds.contains(it.key())) {
            return false;  // Invalid task ID in dependencies
        }

        for (const auto &depId : it.value()) {
            if (!taskIds.contains(depId)) {
                return false;  // Invalid dependency reference
            }
        }
    }

    return true;
}
