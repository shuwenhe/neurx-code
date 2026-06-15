#include "BackgroundAgentManager.h"
#include "SubAgentSystem.h"
#include "AgentScheduler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QThread>
#include <QDateTime>
#include <QUuid>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────────────────
// BackgroundJob Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString BackgroundJob::toJson() const
{
    QJsonObject obj;
    obj["jobId"] = jobId;
    obj["name"] = name;
    obj["description"] = description;
    obj["status"] = static_cast<int>(status);
    obj["statusMessage"] = statusMessage;
    obj["priority"] = priority;
    obj["executionMode"] = executionMode;
    obj["maxRetries"] = maxRetries;
    obj["timeoutPerTaskMs"] = timeoutPerTaskMs;
    obj["totalTasks"] = totalTasks;
    obj["completedTasks"] = completedTasks;
    obj["failedTasks"] = failedTasks;
    obj["progressPercent"] = progressPercent;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    obj["startedAt"] = startedAt.toString(Qt::ISODate);
    obj["completedAt"] = completedAt.toString(Qt::ISODate);
    obj["pausedAt"] = pausedAt.toString(Qt::ISODate);
    obj["finalError"] = finalError;

    // Serialize tasks
    QJsonArray tasksArray;
    for (const auto &task : tasks) {
        tasksArray.append(QJsonDocument::fromJson(task.toJson().toUtf8()).object());
    }
    obj["tasks"] = tasksArray;

    // Serialize results
    QJsonObject resultsObj;
    for (auto it = results.begin(); it != results.end(); ++it) {
        resultsObj[it.key()] = QJsonDocument::fromJson(it.value().toJson().toUtf8()).object();
    }
    obj["results"] = resultsObj;

    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

BackgroundJob BackgroundJob::fromJson(const QString& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();

    BackgroundJob job;
    job.jobId = obj["jobId"].toString();
    job.name = obj["name"].toString();
    job.description = obj["description"].toString();
    job.status = static_cast<JobStatus>(obj["status"].toInt());
    job.statusMessage = obj["statusMessage"].toString();
    job.priority = obj["priority"].toInt();
    job.executionMode = obj["executionMode"].toString();
    job.maxRetries = obj["maxRetries"].toInt();
    job.timeoutPerTaskMs = obj["timeoutPerTaskMs"].toInt();
    job.totalTasks = obj["totalTasks"].toInt();
    job.completedTasks = obj["completedTasks"].toInt();
    job.failedTasks = obj["failedTasks"].toInt();
    job.progressPercent = obj["progressPercent"].toDouble();
    job.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
    job.startedAt = QDateTime::fromString(obj["startedAt"].toString(), Qt::ISODate);
    job.completedAt = QDateTime::fromString(obj["completedAt"].toString(), Qt::ISODate);
    job.pausedAt = QDateTime::fromString(obj["pausedAt"].toString(), Qt::ISODate);
    job.finalError = obj["finalError"].toString();

    // Deserialize tasks
    QJsonArray tasksArray = obj["tasks"].toArray();
    for (const auto &taskVal : tasksArray) {
        job.tasks.append(SubAgentTask::fromJson(taskVal.toObject()));
    }

    // Deserialize results
    QJsonObject resultsObj = obj["results"].toObject();
    for (auto it = resultsObj.begin(); it != resultsObj.end(); ++it) {
        job.results[it.key()] = SubAgentResult::fromJson(it.value().toObject());
    }

    return job;
}

// ──────────────────────────────────────────────────────────────────────────────
// BackgroundAgentManager Implementation
// ──────────────────────────────────────────────────────────────────────────────

BackgroundAgentManager::BackgroundAgentManager(QObject *parent)
    : QObject(parent)
{
    // Create worker thread
    m_workerThread = new QThread(this);
    moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::started, this, &BackgroundAgentManager::processNextJob);
    m_workerThread->start();
}

BackgroundAgentManager::~BackgroundAgentManager()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void BackgroundAgentManager::setSubAgentSystem(SubAgentSystem *system)
{
    m_subAgentSystem = system;
    if (m_subAgentSystem) {
        connect(m_subAgentSystem,
                static_cast<void (SubAgentSystem::*)(const QString&, const SubAgentResult&)>(
                    &SubAgentSystem::taskCompleted),
                this, &BackgroundAgentManager::handleTaskResult);
    }
}

void BackgroundAgentManager::setAgentScheduler(AgentScheduler *scheduler)
{
    m_agentScheduler = scheduler;
}

QString BackgroundAgentManager::submitJob(const BackgroundJob &job)
{
    if (!validateJob(job)) {
        return "";
    }

    BackgroundJob newJob = job;
    if (newJob.jobId.isEmpty()) {
        newJob.jobId = QString("job_%1").arg(++m_sequenceCounter);
    }

    newJob.createdAt = QDateTime::currentDateTime();
    newJob.status = JobStatus::Pending;
    newJob.totalTasks = newJob.tasks.size();

    m_jobs[newJob.jobId] = newJob;
    m_jobQueue.append(newJob.jobId);

    sortJobQueue();
    emit jobSubmitted(newJob.jobId);

    // Trigger processing if not already running
    if (m_currentJobId.isEmpty()) {
        QMetaObject::invokeMethod(this, &BackgroundAgentManager::processNextJob, Qt::QueuedConnection);
    }

    return newJob.jobId;
}

QString BackgroundAgentManager::submitJob(const QString &jobName, const QVector<SubAgentTask> &tasks)
{
    BackgroundJob job;
    job.name = jobName;
    job.tasks = tasks;
    job.totalTasks = tasks.size();

    return submitJob(job);
}

void BackgroundAgentManager::pauseJob(const QString &jobId)
{
    if (!m_jobs.contains(jobId)) {
        return;
    }

    auto &job = m_jobs[jobId];
    job.status = JobStatus::Paused;
    job.pausedAt = QDateTime::currentDateTime();

    // Pause all active tasks
    if (m_subAgentSystem) {
        for (const auto &task : job.tasks) {
            m_subAgentSystem->pauseTask(task.taskId);
        }
    }

    emit jobPaused(jobId);
}

void BackgroundAgentManager::resumeJob(const QString &jobId)
{
    if (!m_jobs.contains(jobId)) {
        return;
    }

    auto &job = m_jobs[jobId];
    if (job.status != JobStatus::Paused) {
        return;
    }

    job.status = JobStatus::Running;

    // Resume all paused tasks
    if (m_subAgentSystem) {
        for (const auto &task : job.tasks) {
            m_subAgentSystem->resumeTask(task.taskId);
        }
    }

    emit jobResumed(jobId);
}

void BackgroundAgentManager::cancelJob(const QString &jobId)
{
    if (!m_jobs.contains(jobId)) {
        return;
    }

    auto &job = m_jobs[jobId];
    
    // Cancel all tasks
    if (m_subAgentSystem) {
        for (const auto &task : job.tasks) {
            m_subAgentSystem->cancelTask(task.taskId);
        }
    }

    job.status = JobStatus::Cancelled;
    m_jobQueue.removeAll(jobId);

    emit jobCancelled(jobId);
}

void BackgroundAgentManager::retryJob(const QString &jobId)
{
    if (!m_jobs.contains(jobId)) {
        return;
    }

    auto &job = m_jobs[jobId];
    if (job.status != JobStatus::Failed) {
        return;
    }

    // Reset job state
    job.status = JobStatus::Pending;
    job.completedTasks = 0;
    job.failedTasks = 0;
    job.progressPercent = 0.0f;
    job.results.clear();
    job.finalError.clear();

    m_jobQueue.append(jobId);
    sortJobQueue();
}

BackgroundJob BackgroundAgentManager::getJob(const QString &jobId) const
{
    if (m_jobs.contains(jobId)) {
        return m_jobs[jobId];
    }

    return BackgroundJob();
}

QVector<BackgroundJob> BackgroundAgentManager::getAllJobs() const
{
    QVector<BackgroundJob> jobs;
    for (const auto &job : m_jobs) {
        jobs.append(job);
    }
    return jobs;
}

QVector<BackgroundJob> BackgroundAgentManager::getJobsByStatus(JobStatus status) const
{
    QVector<BackgroundJob> jobs;
    for (const auto &job : m_jobs) {
        if (job.status == status) {
            jobs.append(job);
        }
    }
    return jobs;
}

void BackgroundAgentManager::onJobCompleted(const QString &jobId, const JobCallback &callback)
{
    m_completionCallbacks[jobId] = callback;
}

void BackgroundAgentManager::onJobProgress(const QString &jobId, const ProgressCallback &callback)
{
    m_progressCallbacks[jobId] = callback;
}

void BackgroundAgentManager::onJobError(const QString &jobId, const BackgroundJobErrorCallback &callback)
{
    m_errorCallbacks[jobId] = callback;
}

int BackgroundAgentManager::getRunningJobCount() const
{
    int count = 0;
    for (const auto &job : m_jobs) {
        if (job.status == JobStatus::Running) {
            ++count;
        }
    }
    return count;
}

int BackgroundAgentManager::getQueuedJobCount() const
{
    return m_jobQueue.size();
}

float BackgroundAgentManager::getAverageJobExecutionTimeMs() const
{
    float totalTime = 0.0f;
    int count = 0;

    for (const auto &job : m_jobs) {
        if (job.status == JobStatus::Completed && job.startedAt.isValid() && job.completedAt.isValid()) {
            totalTime += job.startedAt.msecsTo(job.completedAt);
            ++count;
        }
    }

    return count > 0 ? totalTime / count : 0.0f;
}

void BackgroundAgentManager::saveJobsToDisk(const QString &filePath)
{
    QJsonArray jobsArray;
    for (const auto &job : m_jobs) {
        jobsArray.append(QJsonDocument::fromJson(job.toJson().toUtf8()).object());
    }

    QJsonDocument doc(jobsArray);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void BackgroundAgentManager::loadJobsFromDisk(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonArray jobsArray = doc.array();
    for (const auto &jobVal : jobsArray) {
        BackgroundJob job = BackgroundJob::fromJson(
            QString::fromUtf8(QJsonDocument(jobVal.toObject()).toJson(QJsonDocument::Compact)));
        
        m_jobs[job.jobId] = job;
        
        // Add pending/paused jobs back to queue
        if (job.status == JobStatus::Pending || job.status == JobStatus::Paused) {
            m_jobQueue.append(job.jobId);
        }
    }

    sortJobQueue();
}

void BackgroundAgentManager::processNextJob()
{
    if (m_currentJobId.isEmpty() && !m_jobQueue.isEmpty()) {
        m_currentJobId = m_jobQueue.takeFirst();
        executeJob(m_currentJobId);
    }

    if (!m_currentJobId.isEmpty()) {
        // Schedule next job processing
        QMetaObject::invokeMethod(this, &BackgroundAgentManager::processNextJob,
                                 Qt::QueuedConnection);
    }
}

void BackgroundAgentManager::handleTaskResult(const QString &taskId, const SubAgentResult &result)
{
    // Update current job with task result
    if (!m_jobs.contains(m_currentJobId)) {
        return;
    }

    auto &job = m_jobs[m_currentJobId];
    
    // Find and update task
    for (const auto &task : job.tasks) {
        if (task.taskId == taskId) {
            job.results[taskId] = result;

            if (result.success) {
                job.completedTasks++;
            } else {
                job.failedTasks++;
            }

            job.progressPercent = (float)job.completedTasks / job.totalTasks * 100.0f;
            emit jobProgress(m_currentJobId, static_cast<int>(job.progressPercent));

            triggerCallbacks(m_currentJobId);
            return;
        }
    }
}

void BackgroundAgentManager::updateJobProgress()
{
    if (m_jobs.contains(m_currentJobId)) {
        auto &job = m_jobs[m_currentJobId];
        job.progressPercent = (float)job.completedTasks / job.totalTasks * 100.0f;
        emit jobProgress(m_currentJobId, static_cast<int>(job.progressPercent));
    }
}

void BackgroundAgentManager::executeJob(const QString &jobId)
{
    if (!m_jobs.contains(jobId)) {
        m_currentJobId.clear();
        return;
    }

    auto &job = m_jobs[jobId];
    job.status = JobStatus::Running;
    job.startedAt = QDateTime::currentDateTime();

    emit jobStarted(jobId);

    // Execute tasks based on mode
    if (m_agentScheduler) {
        ScheduleResult result;

        if (job.executionMode == "sequential") {
            result = m_agentScheduler->scheduleMultipleTasks(job.tasks);
        } else {
            result = m_agentScheduler->scheduleMultipleTasks(job.tasks);
        }

        // Update job results
        job.results = result.taskResults;
        job.completedTasks = result.successfulTasks;
        job.failedTasks = result.failedTasks;
        job.progressPercent = 100.0f;

        bool success = result.failedTasks == 0;
        completeJob(jobId, success);
    }

    m_currentJobId.clear();
}

void BackgroundAgentManager::completeJob(const QString &jobId, bool success, const QString &error)
{
    if (!m_jobs.contains(jobId)) {
        return;
    }

    auto &job = m_jobs[jobId];
    job.status = success ? JobStatus::Completed : JobStatus::Failed;
    job.completedAt = QDateTime::currentDateTime();
    
    if (!error.isEmpty()) {
        job.finalError = error;
    }

    triggerCallbacks(jobId);

    if (success) {
        emit jobCompleted(jobId);
    } else {
        emit jobFailed(jobId, job.finalError);
    }
}

void BackgroundAgentManager::updateJobStatus(const QString &jobId, JobStatus status, const QString &message)
{
    if (!m_jobs.contains(jobId)) {
        return;
    }

    m_jobs[jobId].status = status;
    m_jobs[jobId].statusMessage = message;
}

void BackgroundAgentManager::triggerCallbacks(const QString &jobId)
{
    if (m_completionCallbacks.contains(jobId)) {
        m_completionCallbacks[jobId](m_jobs[jobId]);
    }

    if (m_progressCallbacks.contains(jobId)) {
        m_progressCallbacks[jobId](m_jobs[jobId], static_cast<int>(m_jobs[jobId].progressPercent));
    }

    if (m_errorCallbacks.contains(jobId) && m_jobs[jobId].status == JobStatus::Failed) {
        m_errorCallbacks[jobId](m_jobs[jobId], m_jobs[jobId].finalError);
    }
}

void BackgroundAgentManager::sortJobQueue()
{
    // Sort by priority (higher priority first, then by FIFO)
    std::stable_sort(m_jobQueue.begin(), m_jobQueue.end(),
        [this](const QString &a, const QString &b) {
            return m_jobs[a].priority > m_jobs[b].priority;
        });
}

bool BackgroundAgentManager::validateJob(const BackgroundJob &job) const
{
    return !job.tasks.isEmpty();
}
