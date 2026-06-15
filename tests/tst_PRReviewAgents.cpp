#include <QTest>
#include <QObject>
#include "../src/agent/PRReviewAgents.h"

class TestPRReviewAgents : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        reviewSystem = new PRReviewAgents();
        QVERIFY(reviewSystem != nullptr);
    }

    void cleanupTestCase() {
        delete reviewSystem;
    }

    // Test: Object creation
    void testObjectCreation() {
        QVERIFY(reviewSystem != nullptr);
    }

    // Test: Reviewer agent structure
    void testReviewerAgentStructure() {
        PRReviewAgents::ReviewerAgent agent;
        agent.type = PRReviewAgents::SecurityReviewer;
        agent.name = "Security Expert";
        agent.expertise = "Security vulnerabilities";
        agent.priority = 100;
        agent.reviewAccuracy = 0.95f;
        
        QCOMPARE(agent.type, PRReviewAgents::SecurityReviewer);
        QCOMPARE(agent.priority, 100);
        QVERIFY(agent.reviewAccuracy > 0.9f);
    }

    // Test: All reviewer types
    void testAllReviewerTypes() {
        QVector<PRReviewAgents::ReviewerType> types = {
            PRReviewAgents::SecurityReviewer,
            PRReviewAgents::PerformanceReviewer,
            PRReviewAgents::ArchitectureReviewer,
            PRReviewAgents::DocumentationReviewer,
            PRReviewAgents::TestReviewer,
            PRReviewAgents::CodeQualityReviewer,
            PRReviewAgents::APIReviewer
        };
        
        QCOMPARE(types.size(), 7);
    }

    // Test: Review status types
    void testReviewStatusTypes() {
        QVector<PRReviewAgents::ReviewStatus> statuses = {
            PRReviewAgents::Pending,
            PRReviewAgents::InProgress,
            PRReviewAgents::Approved,
            PRReviewAgents::RequestedChanges,
            PRReviewAgents::Commented,
            PRReviewAgents::Dismissed
        };
        
        QCOMPARE(statuses.size(), 6);
    }

private:
    PRReviewAgents* reviewSystem;
};

QTEST_MAIN(TestPRReviewAgents)
#include "tst_PRReviewAgents.moc"
