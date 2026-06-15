#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QDir>
#include <QUuid>

#include "../src/agent/TaskOrchestrator.h"

class TestTaskOrchestrator : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        context = new ContextManager(this);
        orchestrator = new TaskOrchestrator(this);
        orchestrator->setContextManager(context);
    }

    void cleanup()
    {
        cleanupSessionFiles();
    }

    void testStartRecordAndPersistTask()
    {
        context->addFileContext("/workspace/src/main.cpp");
        context->addSelectionContext("int main() { return 0; }");
        context->addNote("Need to update the entry point");

        const QString threadId = QStringLiteral("thread_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        TaskOrchestrator::StartOptions options;
        options.threadId = threadId;
        options.workspacePath = "/workspace";
        options.currentProvider = "openai";
        options.currentModel = "gpt-5";
        options.contextItems = context->exportContextItems();
        options.todoItems = QVariantList{QVariantMap{{QStringLiteral("text"), QStringLiteral("review files")}}};

        const QString started = orchestrator->startTask("Refactor the entry flow", options);
        QCOMPARE(started, threadId);
        QCOMPARE(orchestrator->goal(), QStringLiteral("Refactor the entry flow"));

        const QString userEvent = orchestrator->recordUserMessage("Please update the main function");
        QVERIFY(!userEvent.isEmpty());

        AgentMessage assistant;
        assistant.role = MessageRole::Assistant;
        assistant.content = "I will patch the entry point.";
        assistant.toolCalls = {ToolCall{QStringLiteral("call-1"), QStringLiteral("file_system"), QJsonObject{{QStringLiteral("operation"), QStringLiteral("read_file")}}}};
        orchestrator->recordAssistantMessage(assistant);

        ToolCall call;
        call.id = QStringLiteral("call-1");
        call.name = QStringLiteral("file_system");
        call.arguments = QJsonObject{{QStringLiteral("operation"), QStringLiteral("read_file")}};
        const QString toolEvent = orchestrator->recordToolCall(call);
        QVERIFY(!toolEvent.isEmpty());

        ToolResult result;
        result.callId = call.id;
        result.name = call.name;
        result.content = QStringLiteral("read ok");
        orchestrator->recordToolResult(result);

        orchestrator->recordFileChange(QStringLiteral("write_file"),
                                       {QStringLiteral("/workspace/src/main.cpp")},
                                       {{QStringLiteral("checkpointId"), QStringLiteral("cp-1")}});
        orchestrator->recordCheckpoint(QStringLiteral("cp-1"),
                                      {QStringLiteral("/workspace/src/main.cpp")},
                                      QStringLiteral("before write"));
        const QString contextSnapshotId = orchestrator->recordContextSnapshot(QStringLiteral("after edit"));
        QVERIFY(!contextSnapshotId.isEmpty());

        const TaskSessionSnapshot snapshot = orchestrator->snapshot();
        QCOMPARE(snapshot.threadId, threadId);
        QCOMPARE(snapshot.goal, QStringLiteral("Refactor the entry flow"));
        QVERIFY(snapshot.executionTimeline.size() >= 5);
        QVERIFY(!snapshot.contextItems.isEmpty());
        QVERIFY(TaskSessionStore::loadById(threadId).isValid());
    }

    void testResumeAndForkTask()
    {
        const QString threadId = QStringLiteral("resume_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
        TaskOrchestrator::StartOptions options;
        options.threadId = threadId;
        options.parentThreadId = QStringLiteral("parent-thread");
        options.workspacePath = "/workspace";
        options.contextItems = QVariantList{QVariantMap{{QStringLiteral("id"), QStringLiteral("ctx-1")},
                                                       {QStringLiteral("type"), QStringLiteral("note")},
                                                       {QStringLiteral("source"), QStringLiteral("user")},
                                                       {QStringLiteral("content"), QStringLiteral("keep this in context")},
                                                       {QStringLiteral("priority"), 80}}};

        orchestrator->startTask("Fix the workflow", options);

        TaskOrchestrator second(this);
        ContextManager secondContext(this);
        second.setContextManager(&secondContext);
        QVERIFY(second.resumeTask(threadId).startsWith(QStringLiteral("resume_")));
        QCOMPARE(second.currentThreadId(), threadId);
        QCOMPARE(second.goal(), QStringLiteral("Fix the workflow"));
        QVERIFY(!secondContext.allContextItems().isEmpty());

        const QString forked = second.forkTask(QStringLiteral("branch"));
        QVERIFY(!forked.isEmpty());
        QVERIFY(forked != threadId);
        QCOMPARE(second.parentThreadId(), threadId);
    }

private:
    void cleanupSessionFiles()
    {
        const QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (root.isEmpty()) {
            return;
        }
        QDir sessionDir(QDir(root).filePath(QStringLiteral("sessions")));
        if (!sessionDir.exists()) {
            return;
        }

        const auto ids = sessionDir.entryList({QStringLiteral("thread_*.json"),
                                               QStringLiteral("resume_*.json")}, QDir::Files);
        for (const QString &fileName : ids) {
            sessionDir.remove(fileName);
        }
    }

    ContextManager *context{nullptr};
    TaskOrchestrator *orchestrator{nullptr};
};

QTEST_MAIN(TestTaskOrchestrator)
#include "tst_TaskOrchestrator.moc"

