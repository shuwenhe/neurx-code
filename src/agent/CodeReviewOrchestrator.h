#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include <QMap>
#include <QDateTime>
#include "AgentCoordinator.h"
#include "CodeChangeTracker.h"

/**
 * @class CodeReviewOrchestrator
 * @brief Orchestrates multi-agent code reviews for changesets
 * 
 * Features:
 * - Assign code reviewers (multiple agents)
 * - Parallel code reviews with voting
 * - Review consensus building
 * - Approval/rejection decision
 * - Review comments aggregation
 */

// ──────────────────────────────────────────────────────────────────────────────
// Review Types & Roles
// ──────────────────────────────────────────────────────────────────────────────

enum class ReviewerRole {
    Maintainer,             // Code maintainer (authority)
    Developer,              // Team member (consensus)
    Security,               // Security reviewer
    Performance,            // Performance reviewer
    Architect,              // Architecture reviewer
    QualityAssurance        // QA reviewer
};

enum class ReviewStatus {
    Pending,                // Awaiting review
    InProgress,             // Currently being reviewed
    Approved,               // Changes approved
    ChangesRequested,       // Changes requested
    Commented,              // Comments only (no decision)
    Rejected                // Changes rejected
};

enum class ReviewDecision {
    NoDecision,
    Approve,
    RequestChanges,
    Reject,
    Abstain                 // Agent abstains from voting
};

// ──────────────────────────────────────────────────────────────────────────────
// Review Comment
// ──────────────────────────────────────────────────────────────────────────────

struct ReviewComment {
    QString commentId;
    QString filePath;
    int lineNumber{0};
    QString severity;                   // "info", "warning", "error"
    QString category;                   // "style", "logic", "security", "performance"
    QString comment;
    QString suggestedFix;
    
    QString reviewerAgentId;
    QDateTime createdAt;
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Agent Review Result
// ──────────────────────────────────────────────────────────────────────────────

struct AgentReview {
    QString agentId;
    ReviewerRole role{ReviewerRole::Developer};
    
    ReviewStatus status{ReviewStatus::Pending};
    ReviewDecision decision{ReviewDecision::NoDecision};
    
    QString summary;
    QVector<ReviewComment> comments;
    
    int suggestedChanges{0};
    int blockers{0};
    float approvalScore{0.0f};          // 0-1 confidence in approval
    
    QDateTime startedAt;
    QDateTime completedAt;
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Code Review Result
// ──────────────────────────────────────────────────────────────────────────────

struct CodeReviewResult {
    QString reviewId;
    QString changeSetId;
    
    QVector<AgentReview> agentReviews;
    
    ReviewDecision finalDecision{ReviewDecision::NoDecision};
    float consensusScore{0.0f};         // 0-1 agreement level
    
    int totalComments{0};
    int criticalIssues{0};
    int warnings{0};
    int suggestions{0};
    
    QDateTime reviewStartedAt;
    QDateTime reviewCompletedAt;
    
    QString summary;
    bool canMerge{false};
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Code Review Orchestrator
// ──────────────────────────────────────────────────────────────────────────────

class CodeReviewOrchestrator : public QObject {
    Q_OBJECT

public:
    explicit CodeReviewOrchestrator(QObject *parent = nullptr);
    ~CodeReviewOrchestrator();

    void setAgentCoordinator(AgentCoordinator *coordinator);
    void setCodeChangeTracker(CodeChangeTracker *tracker);

    // Review assignment
    void assignReviewers(const ChangeSet &changeSet, const QStringList &reviewerAgentIds);
    void assignReviewersByRole(const ChangeSet &changeSet, const QVector<ReviewerRole> &roles);
    
    // Review execution
    CodeReviewResult conductReview(const ChangeSet &changeSet);
    CodeReviewResult conductParallelReview(const ChangeSet &changeSet, const QStringList &reviewerIds);
    
    // Review query
    CodeReviewResult getReviewResult(const QString &reviewId) const;
    QVector<CodeReviewResult> getAllReviewResults() const;
    QVector<AgentReview> getReviewsByStatus(ReviewStatus status) const;
    
    // Comment management
    void addReviewComment(const QString &reviewId, const ReviewComment &comment);
    QVector<ReviewComment> getCommentsForFile(const QString &reviewId, const QString &filePath) const;
    
    // Approval decision
    bool isApprovedForMerge(const CodeReviewResult &result) const;
    QString getApprovalSummary(const CodeReviewResult &result) const;
    
    // Statistics
    float calculateConsensusScore(const QVector<ReviewDecision> &decisions) const;
    int countCriticalIssues(const CodeReviewResult &result) const;
    
    // Persistence
    bool saveReviewToFile(const QString &reviewId, const QString &filePath) const;
    bool loadReviewFromFile(const QString &filePath);

signals:
    void reviewStarted(const QString &reviewId, int reviewerCount);
    void reviewerStarted(const QString &reviewId, const QString &agentId);
    void reviewerCompleted(const QString &reviewId, const QString &agentId);
    void reviewCompleted(const QString &reviewId, bool approved);
    void criticalIssueFound(const QString &reviewId, const ReviewComment &comment);

private:
    ReviewDecision aggregateDecisions(const QVector<AgentReview> &reviews) const;
    QString generateReviewSummary(const CodeReviewResult &result) const;
    void categorizeComments(CodeReviewResult &result);
    
    // Member variables
    AgentCoordinator *m_coordinator{nullptr};
    CodeChangeTracker *m_tracker{nullptr};
    
    QMap<QString, CodeReviewResult> m_reviews;
    QMap<QString, QVector<AgentReview>> m_agentReviews;
};
