#include "CodeReviewOrchestrator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QUuid>
#include <algorithm>
#include <numeric>

#include "CodeQualityAnalyzer.h"

// ──────────────────────────────────────────────────────────────────────────────
// ReviewComment Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString ReviewComment::toJson() const
{
    QJsonObject obj;
    obj["commentId"] = commentId;
    obj["filePath"] = filePath;
    obj["lineNumber"] = lineNumber;
    obj["severity"] = severity;
    obj["category"] = category;
    obj["comment"] = comment;
    obj["suggestedFix"] = suggestedFix;
    obj["reviewerAgentId"] = reviewerAgentId;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// ──────────────────────────────────────────────────────────────────────────────
// AgentReview Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString AgentReview::toJson() const
{
    QJsonObject obj;
    obj["agentId"] = agentId;
    obj["role"] = static_cast<int>(role);
    obj["status"] = static_cast<int>(status);
    obj["decision"] = static_cast<int>(decision);
    obj["summary"] = summary;
    obj["suggestedChanges"] = suggestedChanges;
    obj["blockers"] = blockers;
    obj["approvalScore"] = approvalScore;
    obj["startedAt"] = startedAt.toString(Qt::ISODate);
    obj["completedAt"] = completedAt.toString(Qt::ISODate);
    
    // Serialize comments
    QJsonArray commentsArray;
    for (const auto &comment : comments) {
        commentsArray.append(QJsonDocument::fromJson(comment.toJson().toUtf8()).object());
    }
    obj["comments"] = commentsArray;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// ──────────────────────────────────────────────────────────────────────────────
// CodeReviewResult Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString CodeReviewResult::toJson() const
{
    QJsonObject obj;
    obj["reviewId"] = reviewId;
    obj["changeSetId"] = changeSetId;
    obj["finalDecision"] = static_cast<int>(finalDecision);
    obj["consensusScore"] = consensusScore;
    obj["totalComments"] = totalComments;
    obj["criticalIssues"] = criticalIssues;
    obj["warnings"] = warnings;
    obj["suggestions"] = suggestions;
    obj["reviewStartedAt"] = reviewStartedAt.toString(Qt::ISODate);
    obj["reviewCompletedAt"] = reviewCompletedAt.toString(Qt::ISODate);
    obj["summary"] = summary;
    obj["canMerge"] = canMerge;
    
    // Serialize agent reviews
    QJsonArray reviewsArray;
    for (const auto &review : agentReviews) {
        reviewsArray.append(QJsonDocument::fromJson(review.toJson().toUtf8()).object());
    }
    obj["agentReviews"] = reviewsArray;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// ──────────────────────────────────────────────────────────────────────────────
// CodeReviewOrchestrator Implementation
// ──────────────────────────────────────────────────────────────────────────────

CodeReviewOrchestrator::CodeReviewOrchestrator(QObject *parent)
    : QObject(parent)
{
}

CodeReviewOrchestrator::~CodeReviewOrchestrator()
{
}

void CodeReviewOrchestrator::setAgentCoordinator(AgentCoordinator *coordinator)
{
    m_coordinator = coordinator;
}

void CodeReviewOrchestrator::setCodeChangeTracker(CodeChangeTracker *tracker)
{
    m_tracker = tracker;
}

void CodeReviewOrchestrator::assignReviewers(const ChangeSet &changeSet, const QStringList &reviewerAgentIds)
{
    // Create review record
    QString reviewId = QUuid::createUuid().toString();
    
    CodeReviewResult result;
    result.reviewId = reviewId;
    result.changeSetId = changeSet.changeSetId;
    result.reviewStartedAt = QDateTime::currentDateTime();
    
    // Initialize agent reviews
    for (const auto &agentId : reviewerAgentIds) {
        AgentReview review;
        review.agentId = agentId;
        review.role = ReviewerRole::Developer;
        review.status = ReviewStatus::Pending;
        result.agentReviews.append(review);
    }
    
    m_reviews[reviewId] = result;
    
    emit reviewStarted(reviewId, reviewerAgentIds.size());
}

void CodeReviewOrchestrator::assignReviewersByRole(const ChangeSet &changeSet, const QVector<ReviewerRole> &roles)
{
    QStringList agentIds;
    for (int i = 0; i < roles.size(); ++i) {
        const QString roleName = [&]() {
            switch (roles[i]) {
            case ReviewerRole::Maintainer: return QStringLiteral("maintainer");
            case ReviewerRole::Developer: return QStringLiteral("developer");
            case ReviewerRole::Security: return QStringLiteral("security");
            case ReviewerRole::Performance: return QStringLiteral("performance");
            case ReviewerRole::Architect: return QStringLiteral("architect");
            case ReviewerRole::QualityAssurance: return QStringLiteral("qa");
            }
            return QStringLiteral("reviewer");
        }();
        agentIds.append(QStringLiteral("%1-reviewer-%2").arg(roleName).arg(i + 1));
    }
    
    assignReviewers(changeSet, agentIds);
}

CodeReviewResult CodeReviewOrchestrator::conductReview(const ChangeSet &changeSet)
{
    return conductParallelReview(changeSet, {
        QStringLiteral("maintainer-reviewer"),
        QStringLiteral("security-reviewer"),
        QStringLiteral("quality-reviewer")
    });
}

CodeReviewResult CodeReviewOrchestrator::conductParallelReview(const ChangeSet &changeSet, const QStringList &reviewerIds)
{
    QString reviewId = QUuid::createUuid().toString();
    
    CodeReviewResult result;
    result.reviewId = reviewId;
    result.changeSetId = changeSet.changeSetId;
    result.reviewStartedAt = QDateTime::currentDateTime();
    
    emit reviewStarted(reviewId, reviewerIds.size());
    
    CodeQualityAnalyzer analyzer;
    const CodeQualityReport quality = analyzer.analyzeChangeSet(changeSet);

    // Simulate parallel reviews from multiple agents with deterministic outcomes
    for (int i = 0; i < reviewerIds.size(); ++i) {
        const QString &agentId = reviewerIds[i];
        AgentReview review;
        review.agentId = agentId;
        review.status = ReviewStatus::InProgress;
        review.startedAt = QDateTime::currentDateTime();
        review.role = static_cast<ReviewerRole>(i % 6);
        
        emit reviewerStarted(reviewId, agentId);
        
        if (agentId.contains(QStringLiteral("security"), Qt::CaseInsensitive) && quality.criticalIssues > 0) {
            review.status = ReviewStatus::Rejected;
            review.decision = ReviewDecision::Reject;
            review.approvalScore = 0.2f;
            review.summary = QStringLiteral("Security issues require rejection");
            review.blockers = quality.criticalIssues;
        } else if (agentId.contains(QStringLiteral("performance"), Qt::CaseInsensitive)
                   && changeSet.fileChanges.size() > 5) {
            review.status = ReviewStatus::ChangesRequested;
            review.decision = ReviewDecision::RequestChanges;
            review.approvalScore = 0.45f;
            review.summary = QStringLiteral("Change set is too broad for a safe approval");
            review.suggestedChanges = changeSet.fileChanges.size();
        } else if (agentId.contains(QStringLiteral("quality"), Qt::CaseInsensitive) && quality.warnings > 0) {
            review.status = ReviewStatus::Commented;
            review.decision = ReviewDecision::RequestChanges;
            review.approvalScore = 0.55f;
            review.summary = QStringLiteral("Quality warnings should be addressed");
            review.suggestedChanges = quality.warnings;
        } else {
            review.status = ReviewStatus::Approved;
            review.decision = ReviewDecision::Approve;
            review.approvalScore = quality.criticalIssues > 0 ? 0.35f : 0.9f;
            review.summary = QStringLiteral("No blocking issues found");
        }
        review.completedAt = QDateTime::currentDateTime();
        
        result.agentReviews.append(review);
        
        emit reviewerCompleted(reviewId, agentId);
    }
    
    // Calculate final decision
    QVector<ReviewDecision> decisions;
    for (const auto &review : result.agentReviews) {
        decisions.append(review.decision);
    }
    
    result.finalDecision = aggregateDecisions(result.agentReviews);
    result.consensusScore = calculateConsensusScore(decisions);
    result.canMerge = (result.finalDecision == ReviewDecision::Approve);
    result.reviewCompletedAt = QDateTime::currentDateTime();
    result.summary = generateReviewSummary(result);
    categorizeComments(result);
    
    m_reviews[reviewId] = result;
    
    emit reviewCompleted(reviewId, result.canMerge);
    
    return result;
}

CodeReviewResult CodeReviewOrchestrator::getReviewResult(const QString &reviewId) const
{
    auto it = m_reviews.find(reviewId);
    if (it != m_reviews.end()) {
        return it.value();
    }
    return CodeReviewResult();
}

QVector<CodeReviewResult> CodeReviewOrchestrator::getAllReviewResults() const
{
    return m_reviews.values().toVector();
}

QVector<AgentReview> CodeReviewOrchestrator::getReviewsByStatus(ReviewStatus status) const
{
    QVector<AgentReview> result;
    for (const auto &review : m_reviews) {
        for (const auto &agentReview : review.agentReviews) {
            if (agentReview.status == status) {
                result.append(agentReview);
            }
        }
    }
    return result;
}

void CodeReviewOrchestrator::addReviewComment(const QString &reviewId, const ReviewComment &comment)
{
    auto it = m_reviews.find(reviewId);
    if (it != m_reviews.end()) {
        // Find the agent review
        for (auto &agentReview : it.value().agentReviews) {
            if (agentReview.agentId == comment.reviewerAgentId) {
                agentReview.comments.append(comment);
                it.value().totalComments++;
                
                if (comment.severity == "error") {
                    it.value().criticalIssues++;
                    emit criticalIssueFound(reviewId, comment);
                } else if (comment.severity == "warning") {
                    it.value().warnings++;
                }
                
                break;
            }
        }
    }
}

QVector<ReviewComment> CodeReviewOrchestrator::getCommentsForFile(const QString &reviewId, const QString &filePath) const
{
    QVector<ReviewComment> result;
    
    auto it = m_reviews.find(reviewId);
    if (it != m_reviews.end()) {
        for (const auto &agentReview : it.value().agentReviews) {
            for (const auto &comment : agentReview.comments) {
                if (comment.filePath == filePath) {
                    result.append(comment);
                }
            }
        }
    }
    
    return result;
}

bool CodeReviewOrchestrator::isApprovedForMerge(const CodeReviewResult &result) const
{
    return result.canMerge && result.finalDecision == ReviewDecision::Approve;
}

QString CodeReviewOrchestrator::getApprovalSummary(const CodeReviewResult &result) const
{
    QString summary;
    
    int approvals = 0;
    int changeRequests = 0;
    int rejections = 0;
    
    for (const auto &review : result.agentReviews) {
        if (review.decision == ReviewDecision::Approve) {
            approvals++;
        } else if (review.decision == ReviewDecision::RequestChanges) {
            changeRequests++;
        } else if (review.decision == ReviewDecision::Reject) {
            rejections++;
        }
    }
    
    summary += QString("Approvals: %1\n").arg(approvals);
    summary += QString("Changes Requested: %1\n").arg(changeRequests);
    summary += QString("Rejections: %1\n").arg(rejections);
    summary += QString("Consensus: %1%\n").arg(static_cast<int>(result.consensusScore * 100));
    summary += QString("Can Merge: %1\n").arg(result.canMerge ? "Yes" : "No");
    if (!result.summary.trimmed().isEmpty()) {
        summary += QString("Summary: %1\n").arg(result.summary);
    }
    
    return summary;
}

float CodeReviewOrchestrator::calculateConsensusScore(const QVector<ReviewDecision> &decisions) const
{
    if (decisions.isEmpty()) {
        return 0.0f;
    }
    
    int approvals = 0;
    for (const auto &decision : decisions) {
        if (decision == ReviewDecision::Approve) {
            approvals++;
        }
    }
    
    return static_cast<float>(approvals) / decisions.size();
}

int CodeReviewOrchestrator::countCriticalIssues(const CodeReviewResult &result) const
{
    int count = 0;
    for (const auto &review : result.agentReviews) {
        for (const auto &comment : review.comments) {
            if (comment.severity == "error") {
                count++;
            }
        }
    }
    return count;
}

bool CodeReviewOrchestrator::saveReviewToFile(const QString &reviewId, const QString &filePath) const
{
    auto it = m_reviews.find(reviewId);
    if (it == m_reviews.end()) {
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QString jsonStr = it.value().toJson();
    file.write(jsonStr.toUtf8());
    file.close();
    
    return true;
}

bool CodeReviewOrchestrator::loadReviewFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        return false;
    }

    const QJsonObject obj = doc.object();
    CodeReviewResult result;
    result.reviewId = obj.value(QStringLiteral("reviewId")).toString();
    result.changeSetId = obj.value(QStringLiteral("changeSetId")).toString();
    result.finalDecision = static_cast<ReviewDecision>(obj.value(QStringLiteral("finalDecision")).toInt());
    result.consensusScore = static_cast<float>(obj.value(QStringLiteral("consensusScore")).toDouble());
    result.totalComments = obj.value(QStringLiteral("totalComments")).toInt();
    result.criticalIssues = obj.value(QStringLiteral("criticalIssues")).toInt();
    result.warnings = obj.value(QStringLiteral("warnings")).toInt();
    result.suggestions = obj.value(QStringLiteral("suggestions")).toInt();
    result.reviewStartedAt = QDateTime::fromString(obj.value(QStringLiteral("reviewStartedAt")).toString(), Qt::ISODate);
    result.reviewCompletedAt = QDateTime::fromString(obj.value(QStringLiteral("reviewCompletedAt")).toString(), Qt::ISODate);
    result.summary = obj.value(QStringLiteral("summary")).toString();
    result.canMerge = obj.value(QStringLiteral("canMerge")).toBool();

    for (const auto &reviewValue : obj.value(QStringLiteral("agentReviews")).toArray()) {
        if (!reviewValue.isObject()) {
            continue;
        }
        const QJsonObject reviewObj = reviewValue.toObject();
        AgentReview review;
        review.agentId = reviewObj.value(QStringLiteral("agentId")).toString();
        review.role = static_cast<ReviewerRole>(reviewObj.value(QStringLiteral("role")).toInt());
        review.status = static_cast<ReviewStatus>(reviewObj.value(QStringLiteral("status")).toInt());
        review.decision = static_cast<ReviewDecision>(reviewObj.value(QStringLiteral("decision")).toInt());
        review.summary = reviewObj.value(QStringLiteral("summary")).toString();
        review.suggestedChanges = reviewObj.value(QStringLiteral("suggestedChanges")).toInt();
        review.blockers = reviewObj.value(QStringLiteral("blockers")).toInt();
        review.approvalScore = static_cast<float>(reviewObj.value(QStringLiteral("approvalScore")).toDouble());
        review.startedAt = QDateTime::fromString(reviewObj.value(QStringLiteral("startedAt")).toString(), Qt::ISODate);
        review.completedAt = QDateTime::fromString(reviewObj.value(QStringLiteral("completedAt")).toString(), Qt::ISODate);
        result.agentReviews.append(review);
    }

    if (result.reviewId.isEmpty()) {
        result.reviewId = QUuid::createUuid().toString();
    }

    m_reviews[result.reviewId] = result;
    return true;
}

ReviewDecision CodeReviewOrchestrator::aggregateDecisions(const QVector<AgentReview> &reviews) const
{
    if (reviews.isEmpty()) {
        return ReviewDecision::NoDecision;
    }
    
    int approvals = 0;
    int rejections = 0;
    int changeRequests = 0;
    
    for (const auto &review : reviews) {
        if (review.decision == ReviewDecision::Approve) {
            approvals++;
        } else if (review.decision == ReviewDecision::Reject) {
            rejections++;
        } else if (review.decision == ReviewDecision::RequestChanges) {
            changeRequests++;
        }
    }
    
    // Majority wins
    if (rejections > 0) {
        return ReviewDecision::Reject;
    } else if (changeRequests > approvals) {
        return ReviewDecision::RequestChanges;
    } else if (approvals > 0) {
        return ReviewDecision::Approve;
    }
    
    return ReviewDecision::NoDecision;
}

QString CodeReviewOrchestrator::generateReviewSummary(const CodeReviewResult &result) const
{
    QString summary;
    summary += QString("Review ID: %1\n").arg(result.reviewId);
    summary += QString("Agent Reviews: %1\n").arg(result.agentReviews.size());
    summary += QString("Total Comments: %1\n").arg(result.totalComments);
    summary += QString("Critical Issues: %1\n").arg(result.criticalIssues);
    summary += QString("Warnings: %1\n").arg(result.warnings);
    summary += QString("Consensus: %1%\n").arg(static_cast<int>(result.consensusScore * 100));
    
    return summary;
}

void CodeReviewOrchestrator::categorizeComments(CodeReviewResult &result)
{
    for (const auto &review : result.agentReviews) {
        for (const auto &comment : review.comments) {
            if (comment.severity == "error") {
                result.criticalIssues++;
            } else if (comment.severity == "warning") {
                result.warnings++;
            } else {
                result.suggestions++;
            }
        }
    }
}
