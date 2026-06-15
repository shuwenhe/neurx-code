#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>
#include <QDateTime>
#include <QThread>
#include "SubAgentMessage.h"

class SubAgentSystem;
class AgentScheduler;

/**
 * @class BackgroundAgentManager
 * @brief Manages background execution of agent tasks
 * 
 * Features:
 * - Long-running task execution
 * - Job persistence and resume
 * - Progress monitoring
 * - Failure recovery and retry
 * - Resource cleanup
 */

// ──────────────────────────────────────────────────────────────────────────────
// Job Definition
// ──────────────────────────────────────────────────────────────────────────────

enum class JobStatus {
    Pending,            // Not yet started
    Running,            // Currently executing
    Paused,             // Paused by user
    Completed,          // Successfully finished
    Failed,             // Failed with error
    Cancelled           // Cancelled by user
};

struct BackgroundJob {
    QString jobId;                      // Unique job ID
    QString name;
    QString description;
    
    QVector<SubAgentTask> tasks;        // Tasks to execute
    QMap<QString, QStringList> dependencies;  // Task dependencies
    
    JobStatus status{JobStatus::Pending};
    QString statusMessage;
    
    int priority{0};                    // 0=normal, >0=high, <0=low
    
    // Scheduling info
    QString executionMode;              // "sequential", "parallel", "balanced"
    int maxRetries{2};
    int timeoutPerTaskMs{30000};
    
    // Progress tracking
    int totalTasks{0};
    int completedTasks{0};
    int failedTasks{0};
    float progressPercent{0.0f};
    
    // Timestamps
    QDateTime createdAt;
    QDateTime startedAt;
    QDateTime completedAt;
    QDateTime pausedAt;
    
    // Results
    QMap<QString, SubAgentResult> results;
    QString finalError;
    
    // Methods
    QString toJson() const;
    static BackgroundJob fromJson(const QString& json);
};

// ──────────────────────────────────────────────────────────────────────────────
// Job Callback Interface
// ──────────────────────────────────────────────────────────────────────────────

typedef std::function<void(const BackgroundJob&)> JobCallback;
typedef std::function<void(const BackgroundJob&, int percentComplete)> ProgressCallback;
typedef std::function<void(const BackgroundJob&, const QString& error)> BackgroundJobErrorCallback;

// ──────────────────────────────────────────────────────────────────────────────
// BackgroundAgentManager
// ──────────────────────────────────────────────────────────────────────────────

class BackgroundAgentManager : public QObject {
    Q_OBJECT

public:
    explicit BackgroundAgentManager(QObject *parent = nullptr);
    ~BackgroundAgentManager();

    void setSubAgentSystem(SubAgentSystem *system);
    void setAgentScheduler(AgentScheduler *scheduler);

    // Job submission
    QString submitJob(const BackgroundJob &job);
    QString submitJob(const QString &jobName, const QVector<SubAgentTask> &tasks);

    // Job control
    void pauseJob(const QString &jobId);
    void resumeJob(const QString &jobId);
    void cancelJob(const QString &jobId);
    void retryJob(const QString &jobId);

    // Job querying
    BackgroundJob getJob(const QString &jobId) const;
    QVector<BackgroundJob> getAllJobs() const;
    QVector<BackgroundJob> getJobsByStatus(JobStatus status) const;

    // Callbacks
    void onJobCompleted(const QString &jobId, const JobCallback &callback);
    void onJobProgress(const QString &jobId, const ProgressCallback &callback);
    void onJobError(const QString &jobId, const BackgroundJobErrorCallback &callback);

    // Statistics
    int getJobCount() const { return m_jobs.size(); }
    int getRunningJobCount() const;
    int getQueuedJobCount() const;
    float getAverageJobExecutionTimeMs() const;

    // Persistence
    void saveJobsToDisk(const QString &filePath);
    void loadJobsFromDisk(const QString &filePath);

signals:
    // Job lifecycle
    void jobSubmitted(const QString &jobId);
    void jobStarted(const QString &jobId);
    void jobProgress(const QString &jobId, int percentComplete);
    void jobCompleted(const QString &jobId);
    void jobFailed(const QString &jobId, const QString &error);
    void jobCancelled(const QString &jobId);
    void jobPaused(const QString &jobId);
    void jobResumed(const QString &jobId);

private slots:
    void processNextJob();
    void handleTaskResult(const QString &taskId, const SubAgentResult &result);
    void updateJobProgress();

private:
    void executeJob(const QString &jobId);
    void completeJob(const QString &jobId, bool success, const QString &error = "");
    void updateJobStatus(const QString &jobId, JobStatus status, const QString &message = "");
    void triggerCallbacks(const QString &jobId);
    
    void sortJobQueue();
    bool validateJob(const BackgroundJob &job) const;

    // Member variables
    SubAgentSystem *m_subAgentSystem{nullptr};
    AgentScheduler *m_agentScheduler{nullptr};

    QMap<QString, BackgroundJob> m_jobs;
    QStringList m_jobQueue;             // Priority queue of job IDs
    QString m_currentJobId;             // Currently executing job

    QThread *m_workerThread{nullptr};

    // Callbacks
    QMap<QString, JobCallback> m_completionCallbacks;
    QMap<QString, ProgressCallback> m_progressCallbacks;
    QMap<QString, BackgroundJobErrorCallback> m_errorCallbacks;

    int m_sequenceCounter{0};
};
