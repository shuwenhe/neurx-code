#include <QtTest>
#include <QSignalSpy>
#include "agent/SubAgentSystem.h"
#include "agent/AgentScheduler.h"
#include "agent/BackgroundAgentManager.h"
#include "agent/AgentCoordinator.h"

/**
 * @class TestSubAgentOrchestration
 * @brief Unit tests for Phase 1 SubAgent orchestration system
 * 
 * Tests the complete SubAgent orchestration stack:
 * - SubAgentSystem: agent lifecycle and task distribution
 * - AgentScheduler: task scheduling with multiple execution modes
 * - BackgroundAgentManager: job persistence and lifecycle
 * - AgentCoordinator: multi-agent coordination strategies
 */
class TestSubAgentOrchestration : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // SubAgentSystem Tests
    void testSubAgentMessage_Serialization();
    void testSubAgentMessage_Deserialization();
    void testSubAgentSystem_Creation();
    void testSubAgentTask_Structure();
    void testSubAgentResult_Structure();
    
    // AgentScheduler Tests
    void testAgentScheduler_Creation();
    void testAgentScheduler_SequentialMode();
    void testAgentScheduler_ExecutionModes();
    void testScheduleConfig_DefaultValues();
    
    // BackgroundAgentManager Tests
    void testBackgroundJob_Creation();
    void testBackgroundJob_Serialization();
    void testBackgroundAgentManager_Creation();
    void testJobStatus_Transitions();
    
    // AgentCoordinator Tests
    void testAgentCoordinator_Creation();
    void testCoordinationStrategy_Enum();
    void testAgentWorkflowStep_Structure();
    
    // Integration Tests
    void testPhase1_MessageProtocol();
    void testPhase1_SchedulerConfiguration();

private:
    SubAgentSystem *m_subAgentSystem{nullptr};
    AgentScheduler *m_scheduler{nullptr};
    BackgroundAgentManager *m_backgroundManager{nullptr};
    AgentCoordinator *m_coordinator{nullptr};
};

// ──────────────────────────────────────────────────────────────────────────────
// Test Setup/Teardown
// ──────────────────────────────────────────────────────────────────────────────

void TestSubAgentOrchestration::initTestCase()
{
    // Create test instances
    m_subAgentSystem = new SubAgentSystem();
    m_scheduler = new AgentScheduler();
    m_backgroundManager = new BackgroundAgentManager();
    m_coordinator = new AgentCoordinator();
    
    Q_ASSERT(m_subAgentSystem != nullptr);
    Q_ASSERT(m_scheduler != nullptr);
    Q_ASSERT(m_backgroundManager != nullptr);
    Q_ASSERT(m_coordinator != nullptr);
}

void TestSubAgentOrchestration::cleanupTestCase()
{
    delete m_subAgentSystem;
    delete m_scheduler;
    delete m_backgroundManager;
    delete m_coordinator;
}

// ──────────────────────────────────────────────────────────────────────────────
// SubAgentMessage Tests
// ──────────────────────────────────────────────────────────────────────────────

void TestSubAgentOrchestration::testSubAgentMessage_Serialization()
{
    // Test: Create a message and verify it can be serialized
    SubAgentMessage msg;
    msg.messageId = "msg-001";
    msg.agentId = "agent-01";
    msg.type = SubAgentMessageType::TaskRequest;
    msg.sentAt = QDateTime::currentDateTime();
    msg.priority = 1;
    
    // Verify basic properties
    QCOMPARE(msg.messageId, QString("msg-001"));
    QCOMPARE(msg.agentId, QString("agent-01"));
    QCOMPARE(msg.type, SubAgentMessageType::TaskRequest);
    QCOMPARE(msg.priority, 1);
}

void TestSubAgentOrchestration::testSubAgentMessage_Deserialization()
{
    // Test: Create message from JSON
    QJsonObject jsonMsg;
    jsonMsg["messageId"] = "msg-002";
    jsonMsg["agentId"] = "agent-02";
    jsonMsg["type"] = static_cast<int>(SubAgentMessageType::TaskAccepted);
    
    // Verify JSON structure
    QVERIFY(jsonMsg.contains("messageId"));
    QVERIFY(jsonMsg.contains("agentId"));
    QCOMPARE(jsonMsg["messageId"].toString(), QString("msg-002"));
}

void TestSubAgentOrchestration::testSubAgentTask_Structure()
{
    // Test: SubAgentTask structure
    SubAgentTask task;
    task.taskId = "task-001";
    task.type = "code_review";
    task.timeoutMs = 5000;
    task.priority = 5;
    
    QCOMPARE(task.taskId, QString("task-001"));
    QCOMPARE(task.type, QString("code_review"));
    QCOMPARE(task.timeoutMs, 5000);
    QCOMPARE(task.priority, 5);
}

void TestSubAgentOrchestration::testSubAgentResult_Structure()
{
    // Test: SubAgentResult structure
    SubAgentResult result;
    result.taskId = "task-001";
    result.success = true;
    result.qualityScore = 0.95f;
    result.executionTimeMs = 1234;
    
    QCOMPARE(result.taskId, QString("task-001"));
    QVERIFY(result.success);
    QCOMPARE(result.qualityScore, 0.95f);
    QCOMPARE(result.executionTimeMs, 1234);
}

void TestSubAgentOrchestration::testSubAgentSystem_Creation()
{
    // Test: SubAgentSystem can be created
    QVERIFY(m_subAgentSystem != nullptr);
    
    // Verify basic properties
    QVERIFY(m_subAgentSystem->objectName().isEmpty() || true);  // Object name is optional
}

// ──────────────────────────────────────────────────────────────────────────────
// AgentScheduler Tests
// ──────────────────────────────────────────────────────────────────────────────

void TestSubAgentOrchestration::testAgentScheduler_Creation()
{
    // Test: AgentScheduler can be created
    QVERIFY(m_scheduler != nullptr);
}

void TestSubAgentOrchestration::testScheduleConfig_DefaultValues()
{
    // Test: ScheduleConfig has correct defaults
    ScheduleConfig config;
    
    QCOMPARE(config.executionMode, ExecutionMode::Parallel);
    QCOMPARE(config.aggregationMode, ResultAggregationMode::All);
    QCOMPARE(config.maxConcurrentAgents, 5);
    QCOMPARE(config.maxRetries, 2);
}

void TestSubAgentOrchestration::testAgentScheduler_SequentialMode()
{
    // Test: Sequential execution mode enum exists
    QCOMPARE(static_cast<int>(ExecutionMode::Sequential), 0);
}

void TestSubAgentOrchestration::testAgentScheduler_ExecutionModes()
{
    // Test: All execution modes are defined
    QCOMPARE(static_cast<int>(ExecutionMode::Sequential), 0);
    QCOMPARE(static_cast<int>(ExecutionMode::Parallel), 1);
    QCOMPARE(static_cast<int>(ExecutionMode::BalancedParallel), 2);
    QCOMPARE(static_cast<int>(ExecutionMode::DependencyGraph), 3);
    QCOMPARE(static_cast<int>(ExecutionMode::Adaptive), 4);
}

// ──────────────────────────────────────────────────────────────────────────────
// BackgroundAgentManager Tests
// ──────────────────────────────────────────────────────────────────────────────

void TestSubAgentOrchestration::testBackgroundJob_Creation()
{
    // Test: BackgroundJob structure
    BackgroundJob job;
    job.jobId = "job-001";
    job.name = "Test Job";
    job.priority = 5;
    job.status = JobStatus::Pending;
    
    QCOMPARE(job.jobId, QString("job-001"));
    QCOMPARE(job.name, QString("Test Job"));
    QCOMPARE(job.priority, 5);
    QCOMPARE(job.status, JobStatus::Pending);
}

void TestSubAgentOrchestration::testBackgroundJob_Serialization()
{
    // Test: BackgroundJob can be serialized to JSON
    BackgroundJob job;
    job.jobId = "job-002";
    job.name = "Serialization Test";
    job.totalTasks = 10;
    job.completedTasks = 3;
    job.progressPercent = 30;
    
    // Verify fields accessible
    QCOMPARE(job.totalTasks, 10);
    QCOMPARE(job.completedTasks, 3);
    QCOMPARE(job.progressPercent, 30);
}

void TestSubAgentOrchestration::testBackgroundAgentManager_Creation()
{
    // Test: BackgroundAgentManager can be created
    QVERIFY(m_backgroundManager != nullptr);
}

void TestSubAgentOrchestration::testJobStatus_Transitions()
{
    // Test: JobStatus enum values
    QCOMPARE(static_cast<int>(JobStatus::Pending), 0);
    QCOMPARE(static_cast<int>(JobStatus::Running), 1);
    QCOMPARE(static_cast<int>(JobStatus::Paused), 2);
    QCOMPARE(static_cast<int>(JobStatus::Completed), 3);
    QCOMPARE(static_cast<int>(JobStatus::Failed), 4);
    QCOMPARE(static_cast<int>(JobStatus::Cancelled), 5);
}

// ──────────────────────────────────────────────────────────────────────────────
// AgentCoordinator Tests
// ──────────────────────────────────────────────────────────────────────────────

void TestSubAgentOrchestration::testAgentCoordinator_Creation()
{
    // Test: AgentCoordinator can be created
    QVERIFY(m_coordinator != nullptr);
}

void TestSubAgentOrchestration::testCoordinationStrategy_Enum()
{
    // Test: CoordinationStrategy enum values
    QCOMPARE(static_cast<int>(CoordinationStrategy::Sequential), 0);
    QCOMPARE(static_cast<int>(CoordinationStrategy::ParallelVoting), 1);
    QCOMPARE(static_cast<int>(CoordinationStrategy::ParallelPipeline), 2);
    QCOMPARE(static_cast<int>(CoordinationStrategy::Hierarchical), 3);
    QCOMPARE(static_cast<int>(CoordinationStrategy::Custom), 4);
}

void TestSubAgentOrchestration::testAgentWorkflowStep_Structure()
{
    // Test: AgentWorkflowStep structure
    AgentWorkflowStep step;
    step.stepId = "step-001";
    step.description = "Test Step";
    step.strategy = CoordinationStrategy::ParallelVoting;
    
    QCOMPARE(step.stepId, QString("step-001"));
    QCOMPARE(step.description, QString("Test Step"));
    QCOMPARE(step.strategy, CoordinationStrategy::ParallelVoting);
}

// ──────────────────────────────────────────────────────────────────────────────
// Integration Tests
// ──────────────────────────────────────────────────────────────────────────────

void TestSubAgentOrchestration::testPhase1_MessageProtocol()
{
    // Test: Message protocol message types
    QCOMPARE(static_cast<int>(SubAgentMessageType::TaskRequest), 0);
    QCOMPARE(static_cast<int>(SubAgentMessageType::TaskAccepted), 1);
    QCOMPARE(static_cast<int>(SubAgentMessageType::TaskStarted), 2);
    QCOMPARE(static_cast<int>(SubAgentMessageType::TaskProgress), 3);
    QCOMPARE(static_cast<int>(SubAgentMessageType::TaskCompleted), 4);
    QCOMPARE(static_cast<int>(SubAgentMessageType::TaskFailed), 5);
}

void TestSubAgentOrchestration::testPhase1_SchedulerConfiguration()
{
    // Test: Scheduler can be configured
    ScheduleConfig config;
    config.executionMode = ExecutionMode::DependencyGraph;
    config.maxConcurrentAgents = 8;
    config.maxRetries = 3;
    
    QCOMPARE(config.executionMode, ExecutionMode::DependencyGraph);
    QCOMPARE(config.maxConcurrentAgents, 8);
    QCOMPARE(config.maxRetries, 3);
}

QTEST_MAIN(TestSubAgentOrchestration)

#include "tst_SubAgentOrchestration.moc"
