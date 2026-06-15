#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>

/**
 * @class SubAgentMessage
 * @brief Protocol for communication between parent agent and sub-agents
 * 
 * Supports task dispatching, result collection, and event streaming
 */

// ──────────────────────────────────────────────────────────────────────────────
// Message Types
// ──────────────────────────────────────────────────────────────────────────────

enum class SubAgentMessageType {
    TaskRequest,        // Parent → SubAgent: task to execute
    TaskAccepted,       // SubAgent → Parent: accepted the task
    TaskStarted,        // SubAgent → Parent: execution started
    TaskProgress,       // SubAgent → Parent: progress update
    TaskCompleted,      // SubAgent → Parent: task done with result
    TaskFailed,         // SubAgent → Parent: task failed
    TaskCancelled,      // Parent → SubAgent or SubAgent → Parent: cancel task
    PauseRequest,       // Parent → SubAgent: pause execution
    ResumeRequest,      // Parent → SubAgent: resume execution
    HealthCheck,        // Parent → SubAgent: are you alive?
    HealthResponse,     // SubAgent → Parent: alive and status
    LogEvent,           // SubAgent → Parent: log message
    MetricsReport       // SubAgent → Parent: performance metrics
};

// ──────────────────────────────────────────────────────────────────────────────
// Core Message Structure
// ──────────────────────────────────────────────────────────────────────────────

struct SubAgentMessage {
    // Message identity
    QString messageId;                  // Unique message ID
    QString agentId;                    // Sub-agent ID
    SubAgentMessageType type;
    
    // Timestamps
    QDateTime sentAt;
    QDateTime receivedAt;
    
    // Payload
    QJsonObject payload;                // Message-specific data
    
    // Priority and metadata
    int priority{0};                    // 0=normal, >0=urgent, <0=low
    QJsonObject metadata;               // Custom metadata
    
    // Constructors
    SubAgentMessage();
    SubAgentMessage(const QString& agentId, SubAgentMessageType type);
    
    // Helpers
    QString toJsonString() const;
    static SubAgentMessage fromJsonString(const QString& json);
};

// ──────────────────────────────────────────────────────────────────────────────
// Task Request/Response Structures
// ──────────────────────────────────────────────────────────────────────────────

struct SubAgentTask {
    QString taskId;                     // Unique task ID
    QString type;                       // Task type (e.g., "code_review", "security_scan")
    QString description;
    
    QJsonObject parameters;             // Task-specific parameters
    QStringList dependencies;           // IDs of tasks this depends on
    
    int priority{0};
    int timeoutMs{30000};               // 30s default timeout
    
    QDateTime createdAt;
    QDateTime scheduledAt;
    
    // Methods
    QString toJson() const;
    static SubAgentTask fromJson(const QJsonObject& obj);
};

struct SubAgentResult {
    QString taskId;
    QString agentId;
    bool success{false};
    QString errorMessage;
    
    QJsonObject data;                   // Result payload
    QJsonArray intermediateResults;     // Streaming results during execution
    
    // Performance metrics
    int executionTimeMs{0};
    int tokensUsed{0};
    float qualityScore{0.0f};           // 0-1
    
    QDateTime completedAt;
    
    // Methods
    QString toJson() const;
    static SubAgentResult fromJson(const QJsonObject& obj);
};

// ──────────────────────────────────────────────────────────────────────────────
// Progress and Status Structures
// ──────────────────────────────────────────────────────────────────────────────

struct SubAgentProgress {
    QString taskId;
    int percentComplete{0};             // 0-100
    QString currentStage;
    QString statusMessage;
    
    QStringList completedSteps;
    QStringList remainingSteps;
    
    int estimatedRemainingMs{0};
    
    QString toJson() const;
};

struct SubAgentHealthStatus {
    QString agentId;
    bool isAlive{true};
    bool isIdle{true};
    
    int runningTaskCount{0};
    int totalTasksProcessed{0};
    float cpuUsagePercent{0.0f};
    float memoryUsageMb{0.0f};
    
    QString lastErrorMessage;
    QDateTime lastActivityAt;
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Multi-Agent Voting/Consensus Structure
// ──────────────────────────────────────────────────────────────────────────────

struct SubAgentVote {
    QString agentId;
    QString decision;                   // e.g., "approved", "rejected", "needs_review"
    float confidence{0.0f};             // 0-1
    QString justification;
    
    QJsonObject metadata;
};

struct ConsensusResult {
    QVector<SubAgentVote> votes;
    QString finalDecision;              // Majority vote
    float consensusStrength{0.0f};      // 0-1, how strong is consensus
    QString reasoning;
    
    // Helper: get votes by decision
    QVector<SubAgentVote> getVotesFor(const QString& decision) const;
    int getVoteCount(const QString& decision) const;
};
