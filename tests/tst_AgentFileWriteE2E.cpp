#include <QTest>
#include <QObject>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "../src/agent/AgentEngine.h"
#include "../src/agent/AgentToolRegistry.h"
#include "../src/llm/LLMProvider.h"
#include "../src/tools/AgentFileWriterTool.h"

class FakeFileWriteProvider final : public LLMProvider {
    Q_OBJECT

public:
    QString providerId() const override { return QStringLiteral("ollama"); }
    QString displayName() const override { return QStringLiteral("Fake Ollama"); }
    QStringList availableModels() const override { return {QStringLiteral("fake-e2e-model")}; }
    void cancel() override {}

    void sendRequest(const LLMRequest &request) override
    {
        ++requestCount;

        const QString toolsJson = QString::fromUtf8(
            QJsonDocument(request.tools).toJson(QJsonDocument::Compact));
        if (toolsJson.contains(QStringLiteral("agent_file_writer")))
            sawAgentFileWriterSchema = true;

        if (requestCount == 1) {
            firstRequestToolCount = request.tools.size();

            LLMResponse response;
            response.stopReason = QStringLiteral("tool_use");
            response.message.role = MessageRole::Assistant;
            response.message.content = QStringLiteral("I will create the file now.");

            ToolCall call;
            call.id = QStringLiteral("tool-call-1");
            call.name = QStringLiteral("agent_file_writer");
            call.arguments.insert(QStringLiteral("path"), relativeOutputPath);
            call.arguments.insert(QStringLiteral("content"), fileContent);
            call.arguments.insert(QStringLiteral("create_dirs"), true);
            response.message.toolCalls.append(call);

            emit responseComplete(response);
            return;
        }

        if (requestCount == 2) {
            for (const AgentMessage &message : request.messages) {
                if (!message.hasToolResults())
                    continue;
                for (const ToolResult &result : message.toolResults) {
                    if (result.callId == QStringLiteral("tool-call-1")
                        && result.name == QStringLiteral("agent_file_writer")
                        && !result.isError) {
                        sawSuccessfulToolResult = true;
                    }
                }
            }

            LLMResponse response;
            response.stopReason = QStringLiteral("end_turn");
            response.message.role = MessageRole::Assistant;
            response.message.content = QStringLiteral("File created successfully.");
            emit responseComplete(response);
            return;
        }

        emit requestError(QStringLiteral("Unexpected extra request in FakeFileWriteProvider"));
    }

    int requestCount{0};
    int firstRequestToolCount{0};
    bool sawAgentFileWriterSchema{false};
    bool sawSuccessfulToolResult{false};
    QString relativeOutputPath;
    QString fileContent;
};

class TestAgentFileWriteE2E : public QObject {
    Q_OBJECT

private:
    static QString readFile(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        return QString::fromUtf8(file.readAll());
    }

private slots:
    void testAgentCreatesAndWritesFile()
    {
        qRegisterMetaType<ToolResult>("ToolResult");
        qRegisterMetaType<AgentMessage>("AgentMessage");

        QTemporaryDir workspace;
        QVERIFY2(workspace.isValid(), "Temporary workspace should be created");

        const QString relativePath = QStringLiteral("src/hello.cc");
        const QString absolutePath = QDir(workspace.path()).filePath(relativePath);
        const QString fileContent = QStringLiteral(
            "#include <iostream>\n"
            "\n"
            "int main()\n"
            "{\n"
            "    std::cout << \"agent e2e\" << std::endl;\n"
            "    return 0;\n"
            "}\n");

        AgentToolRegistry registry;
        registry.registerTool(new AgentFileWriterTool(workspace.path(), &registry));

        FakeFileWriteProvider provider;
        provider.relativeOutputPath = relativePath;
        provider.fileContent = fileContent;

        AgentEngine engine;
        engine.setProvider(&provider);
        engine.setToolRegistry(&registry);
        engine.setWorkspaceRoot(workspace.path());
        engine.setActiveModel(QStringLiteral("fake-e2e-model"));
        engine.setAutoApproveTools(true);

        QSignalSpy turnCompleteSpy(&engine, &AgentEngine::turnComplete);
        QSignalSpy toolFinishedSpy(&engine, &AgentEngine::toolFinished);
        QSignalSpy errorSpy(&engine, &AgentEngine::errorOccurred);

        engine.submitUserMessage(QStringLiteral("Create src/hello.cc and write a simple C++ program."));

        QTRY_COMPARE_WITH_TIMEOUT(turnCompleteSpy.count(), 1, 10000);
        QCOMPARE(errorSpy.count(), 0);
        QCOMPARE(provider.requestCount, 2);
        QVERIFY(provider.sawAgentFileWriterSchema);
        QVERIFY(provider.firstRequestToolCount > 0);
        QVERIFY(provider.sawSuccessfulToolResult);

        QCOMPARE(toolFinishedSpy.count(), 1);
        const QList<QVariant> toolArgs = toolFinishedSpy.takeFirst();
        QVERIFY(toolArgs.size() == 1);
        const ToolResult toolResult = toolArgs.at(0).value<ToolResult>();
        QCOMPARE(toolResult.name, QStringLiteral("agent_file_writer"));
        QVERIFY(!toolResult.isError);

        QVERIFY2(QFile::exists(absolutePath), qPrintable(QStringLiteral("Expected file missing: %1").arg(absolutePath)));
        QCOMPARE(readFile(absolutePath), fileContent);
    }
};

QTEST_MAIN(TestAgentFileWriteE2E)
#include "tst_AgentFileWriteE2E.moc"
