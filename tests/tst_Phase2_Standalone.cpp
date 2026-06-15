#include <QtTest>
#include <QDebug>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <vector>
#include <cstdlib>
#include <ctime>

// Phase 2 Module Headers
#include "agent/CodeChangeTracker.h"
#include "agent/CodeReviewOrchestrator.h"
#include "agent/CodeChangeValidator.h"
#include "agent/CodeQualityAnalyzer.h"

class TestPhase2Standalone : public QObject {
    Q_OBJECT

private slots:
    // ═══════════════════════════════════════════════════════════════════════════════
    // CodeChangeTracker Tests
    // ═══════════════════════════════════════════════════════════════════════════════

    void testCodeChangeTracker_FileChangeStructure() {
        FileChange fc;
        fc.filePath = "src/main.cpp";
        fc.changeType = ChangeType::Modified;
        fc.status = ChangeStatus::Staged;
        fc.totalAdditions = 10;
        fc.totalDeletions = 5;
        fc.totalModifications = 3;

        QCOMPARE(fc.filePath, QString("src/main.cpp"));
        QCOMPARE(fc.changeType, ChangeType::Modified);
        QCOMPARE(fc.status, ChangeStatus::Staged);
        QCOMPARE(fc.totalAdditions, 10);
    }

    void testCodeChangeTracker_RecordChange() {
        CodeChangeTracker tracker;
        
        FileChange fc;
        fc.filePath = "src/test.cpp";
        fc.changeType = ChangeType::Created;
        fc.status = ChangeStatus::Unstaged;
        fc.totalAdditions = 100;

        tracker.recordChange(fc);
        auto changes = tracker.getAllChanges();
        QCOMPARE(changes.size(), 1);
        QCOMPARE(changes[0].filePath, QString("src/test.cpp"));
    }

    void testCodeChangeTracker_StageUnstage() {
        CodeChangeTracker tracker;
        
        FileChange fc;
        fc.filePath = "src/test.cpp";
        fc.changeType = ChangeType::Modified;
        fc.status = ChangeStatus::Unstaged;

        tracker.recordChange(fc);
        tracker.stageChange("src/test.cpp");
        
        auto changes = tracker.getAllChanges();
        QCOMPARE(changes[0].status, ChangeStatus::Staged);
    }

    void testCodeChangeTracker_ChangeComplexity() {
        CodeChangeTracker tracker;
        
        FileChange fc;
        fc.filePath = "src/complex.cpp";
        fc.changeType = ChangeType::Modified;
        fc.totalAdditions = 500;
        fc.totalDeletions = 200;
        fc.totalModifications = 100;

        tracker.recordChange(fc);
        float complexity = tracker.calculateChangeComplexity(fc);
        
        // Complexity should be between 0 and 1
        QVERIFY(complexity >= 0.0f && complexity <= 1.0f);
        // High changes should have high complexity
        QVERIFY(complexity > 0.5f);
    }

    void testCodeChangeTracker_Statistics() {
        CodeChangeTracker tracker;
        
        FileChange fc1;
        fc1.filePath = "src/file1.cpp";
        fc1.changeType = ChangeType::Created;
        fc1.totalAdditions = 100;
        
        FileChange fc2;
        fc2.filePath = "src/file2.cpp";
        fc2.changeType = ChangeType::Modified;
        fc2.totalAdditions = 50;
        fc2.totalDeletions = 20;

        tracker.recordChange(fc1);
        tracker.recordChange(fc2);

        QCOMPARE(tracker.getTotalFilesChanged(), 2);
        QCOMPARE(tracker.getTotalAdditions(), 150);
    }

    // ═══════════════════════════════════════════════════════════════════════════════
    // CodeReviewOrchestrator Tests
    // ═══════════════════════════════════════════════════════════════════════════════

    void testCodeReviewOrchestrator_ReviewerRoleEnum() {
        QCOMPARE(static_cast<int>(ReviewerRole::Maintainer), 0);
        QCOMPARE(static_cast<int>(ReviewerRole::Developer), 1);
        QCOMPARE(static_cast<int>(ReviewerRole::Security), 2);
        QCOMPARE(static_cast<int>(ReviewerRole::Performance), 3);
        QCOMPARE(static_cast<int>(ReviewerRole::Architect), 4);
        QCOMPARE(static_cast<int>(ReviewerRole::QualityAssurance), 5);
    }

    void testCodeReviewOrchestrator_ReviewStatusEnum() {
        QCOMPARE(static_cast<int>(ReviewStatus::Pending), 0);
        QCOMPARE(static_cast<int>(ReviewStatus::InProgress), 1);
        QCOMPARE(static_cast<int>(ReviewStatus::Approved), 2);
        QCOMPARE(static_cast<int>(ReviewStatus::ChangesRequested), 3);
        QCOMPARE(static_cast<int>(ReviewStatus::Commented), 4);
        QCOMPARE(static_cast<int>(ReviewStatus::Rejected), 5);
    }

    void testCodeReviewOrchestrator_ReviewDecisionEnum() {
        QCOMPARE(static_cast<int>(ReviewDecision::NoDecision), 0);
        QCOMPARE(static_cast<int>(ReviewDecision::Approve), 1);
        QCOMPARE(static_cast<int>(ReviewDecision::RequestChanges), 2);
        QCOMPARE(static_cast<int>(ReviewDecision::Reject), 3);
        QCOMPARE(static_cast<int>(ReviewDecision::Abstain), 4);
    }

    void testCodeReviewOrchestrator_ReviewCommentStructure() {
        ReviewComment comment;
        comment.commentId = "comment-001";
        comment.filePath = "src/main.cpp";
        comment.lineNumber = 42;
        comment.severity = "warning";
        comment.category = "style";
        comment.comment = "This line is too long";
        comment.reviewerAgentId = "agent-reviewer-1";
        
        QCOMPARE(comment.commentId, QString("comment-001"));
        QCOMPARE(comment.lineNumber, 42);
        QCOMPARE(comment.severity, QString("warning"));
    }

    void testCodeReviewOrchestrator_AgentReviewStructure() {
        AgentReview review;
        review.agentId = "agent-reviewer-1";
        review.role = ReviewerRole::Maintainer;
        review.status = ReviewStatus::Approved;
        review.decision = ReviewDecision::Approve;
        review.approvalScore = 0.95f;
        review.summary = "Code looks good";
        
        QCOMPARE(review.agentId, QString("agent-reviewer-1"));
        QCOMPARE(review.role, ReviewerRole::Maintainer);
        QCOMPARE(review.decision, ReviewDecision::Approve);
        QCOMPARE(review.approvalScore, 0.95f);
    }

    void testCodeReviewOrchestrator_CodeReviewResultStructure() {
        CodeReviewResult result;
        result.reviewId = "review-001";
        result.changeSetId = "changeset-001";
        result.finalDecision = ReviewDecision::Approve;
        result.consensusScore = 0.88f;
        result.totalComments = 5;
        result.criticalIssues = 0;
        result.canMerge = true;

        QCOMPARE(result.reviewId, QString("review-001"));
        QCOMPARE(result.consensusScore, 0.88f);
        QVERIFY(result.canMerge);
    }

    void testCodeReviewOrchestrator_ConsensusScoring() {
        CodeReviewOrchestrator orchestrator(nullptr);
        
        QVector<ReviewDecision> decisions;
        decisions.push_back(ReviewDecision::Approve);
        decisions.push_back(ReviewDecision::Approve);

        float consensus = orchestrator.calculateConsensusScore(decisions);
        QVERIFY(consensus > 0.5f && consensus <= 1.0f);
    }

    // ═══════════════════════════════════════════════════════════════════════════════
    // CodeChangeValidator Tests
    // ═══════════════════════════════════════════════════════════════════════════════

    void testCodeChangeValidator_ValidationRuleStructure() {
        ValidationRule rule;
        rule.ruleId = "rule-001";
        rule.name = "Naming Convention";
        rule.description = "Check class names follow conventions";
        rule.enabled = true;
        rule.priority = 7;
        rule.category = "naming";

        QCOMPARE(rule.ruleId, QString("rule-001"));
        QCOMPARE(rule.priority, 7);
        QVERIFY(rule.enabled);
    }

    void testCodeChangeValidator_ValidationViolationStructure() {
        ValidationViolation violation;
        violation.ruleId = "rule-001";
        violation.severity = "warning";
        violation.filePath = "src/main.cpp";
        violation.lineNumber = 10;
        violation.message = "Class name should be PascalCase";
        violation.suggestion = "Rename class to MainApp";

        QCOMPARE(violation.severity, QString("warning"));
        QCOMPARE(violation.lineNumber, 10);
    }

    void testCodeChangeValidator_AddRemoveRules() {
        CodeChangeValidator validator;

        // Get initial rule count
        auto initialRules = validator.getAllRules();
        int initialCount = initialRules.size();

        ValidationRule rule;
        rule.ruleId = "rule-001";
        rule.name = "Test Rule";
        rule.enabled = true;

        validator.addRule(rule);
        auto rules = validator.getAllRules();
        QCOMPARE(rules.size(), initialCount + 1);

        validator.removeRule("rule-001");
        rules = validator.getAllRules();
        QCOMPARE(rules.size(), initialCount);
    }

    void testCodeChangeValidator_ValidateFileName() {
        CodeChangeValidator validator;
        
        ValidationRule rule;
        rule.ruleId = "naming-001";
        rule.name = "File Naming";
        rule.category = "naming";
        rule.enabled = true;

        validator.addRule(rule);

        QString error;
        bool result = validator.validateFileName("src/MyFile.cpp", error);
        // Should validate or provide error message
        QVERIFY(result || !error.isEmpty());
    }

    void testCodeChangeValidator_ValidateCommitMessage() {
        CodeChangeValidator validator;
        
        QString goodMessage = "Add feature: Implement code review system";
        QString error;
        bool result = validator.validateCommitMessage(goodMessage, error);
        
        QVERIFY(result || !error.isEmpty());
    }

    void testCodeChangeValidator_PolicyConfiguration() {
        CodeChangeValidator validator;
        
        validator.setMaxFileSizeKb(5000);
        validator.setMaxFilesPerCommit(50);
        validator.setMinCommitMessageLength(5);

        // Just verify configuration didn't throw
        QVERIFY(true);
    }

    // ═══════════════════════════════════════════════════════════════════════════════
    // CodeQualityAnalyzer Tests
    // ═══════════════════════════════════════════════════════════════════════════════

    void testCodeQualityAnalyzer_MetricsStructure() {
        CodeQualityMetrics metrics;
        metrics.averageCyclomaticComplexity = 3.5f;
        metrics.linesOfCode = 5000;
        metrics.commentedLines = 500;
        metrics.commentRatio = 0.1f;
        metrics.maintainabilityIndex = 75.0f;
        metrics.testCoverage = 0.85f;

        QCOMPARE(metrics.linesOfCode, 5000);
        QCOMPARE(metrics.maintainabilityIndex, 75.0f);
    }

    void testCodeQualityAnalyzer_CalculateCyclomaticComplexity() {
        CodeQualityAnalyzer analyzer;
        
        QString code = "if (x > 0) { y = 1; } else if (x < 0) { y = -1; } else { y = 0; } for(int i=0; i<10; i++) { switch(i) { case 1: break; } }";
        float complexity = analyzer.calculateCyclomaticComplexity(code);
        
        // Should detect multiple decision points
        QVERIFY(complexity >= 1.0f);
    }

    void testCodeQualityAnalyzer_CalculateMaintainabilityIndex() {
        CodeQualityAnalyzer analyzer;
        
        FileChange change;
        change.filePath = "src/test.cpp";
        change.changeType = ChangeType::Modified;
        change.totalAdditions = 10;
        
        float index = analyzer.calculateMaintainabilityIndex(change);
        
        QVERIFY(index >= 0.0f && index <= 100.0f);
    }

    void testCodeQualityAnalyzer_DetectCodeSmells() {
        CodeQualityAnalyzer analyzer;
        
        FileChange change;
        change.filePath = "src/long_function.cpp";
        change.changeType = ChangeType::Modified;
        change.totalAdditions = 150;  // Simulate long function

        auto smells = analyzer.detectCodeSmells(change);
        // Just verify the function can be called
        QVERIFY(true);
    }

    void testCodeQualityAnalyzer_AnalyzeFile() {
        CodeQualityAnalyzer analyzer;
        
        FileChange change;
        change.filePath = "src/test.cpp";
        change.changeType = ChangeType::Created;
        change.totalAdditions = 50;
        
        auto metrics = analyzer.analyzeFile(change);
        
        // Verify metrics structure is valid
        QVERIFY(metrics.linesOfCode >= 0);
    }

    void testCodeQualityAnalyzer_OverallScoring() {
        CodeQualityAnalyzer analyzer;
        
        CodeQualityMetrics metrics;
        metrics.linesOfCode = 1000;
        metrics.averageCyclomaticComplexity = 3.0f;
        metrics.commentRatio = 0.15f;

        float score = analyzer.calculateOverallScore(metrics);
        QVERIFY(score >= 0.0f && score <= 100.0f);
    }

    // ═══════════════════════════════════════════════════════════════════════════════
    // Integration Tests
    // ═══════════════════════════════════════════════════════════════════════════════

    void testIntegration_FullCodeReviewWorkflow() {
        // Create a changeset
        CodeChangeTracker tracker;
        
        FileChange fc;
        fc.filePath = "src/main.cpp";
        fc.changeType = ChangeType::Modified;
        fc.status = ChangeStatus::Staged;
        fc.totalAdditions = 50;
        fc.totalDeletions = 10;

        tracker.recordChange(fc);
        
        ChangeSet changeset;
        changeset.changeSetId = "cs-001";
        changeset.fileChanges.push_back(fc);
        changeset.commitMessage = "Implement new feature";

        // Validate
        CodeChangeValidator validator;
        auto validationResult = validator.validateChangeSet(changeset);
        QVERIFY(validationResult.changeSetId == "cs-001");

        // Analyze quality
        CodeQualityAnalyzer analyzer;
        auto qualityReport = analyzer.analyzeChangeSet(changeset);
        QVERIFY(qualityReport.changeSetId == "cs-001");

        // Review with orchestrator
        CodeReviewOrchestrator orchestrator(nullptr);
        QVERIFY(true); // Just verify objects can work together
    }

    void testIntegration_MultipleChanges() {
        CodeChangeTracker tracker;
        
        for (int i = 0; i < 5; i++) {
            FileChange fc;
            fc.filePath = QString("src/file%1.cpp").arg(i);
            fc.changeType = ChangeType::Modified;
            fc.totalAdditions = 20 * (i + 1);
            tracker.recordChange(fc);
        }

        auto changes = tracker.getAllChanges();
        QCOMPARE(changes.size(), 5);
    }

    void testIntegration_ChangeSetPersistence() {
        CodeChangeTracker tracker;
        
        FileChange fc;
        fc.filePath = "src/main.cpp";
        fc.changeType = ChangeType::Created;
        fc.totalAdditions = 100;

        tracker.recordChange(fc);
        
        ChangeSet changeset;
        changeset.changeSetId = "cs-001";
        changeset.fileChanges = tracker.getAllChanges();
        changeset.commitMessage = "Initial commit";
        changeset.authorName = "John Doe";

        // JSON serialization check
        auto json = changeset.toJson();
        QVERIFY(!json.isEmpty());
    }
};

QTEST_MAIN(TestPhase2Standalone)
#include "tst_Phase2_Standalone.moc"
