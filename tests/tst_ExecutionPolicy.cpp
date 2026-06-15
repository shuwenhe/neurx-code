#include <QTest>
#include <QObject>

#include "../src/agent/ExecutionStrategyManager.h"
#include "../src/sandbox/DefaultSandboxManager.h"

class TestExecutionPolicy : public QObject {
    Q_OBJECT

private slots:
    void testDestructiveShellCommandRequiresApproval()
    {
        ExecutionStrategyManager manager;

        QJsonObject params;
        params[QStringLiteral("command")] = QStringLiteral("rm -rf /tmp/demo");

        const RiskAssessment risk = manager.assessToolRisk(QStringLiteral("run_command"), params);
        QVERIFY(risk.score >= 85);
        QCOMPARE(risk.level, QStringLiteral("critical"));

        const ExecutionStrategy strategy = manager.getStrategy(QStringLiteral("safe"));
        QVERIFY(manager.needsApproval(risk, strategy));
    }

    void testSandboxWriteAccessRequiresExplicitAllowance()
    {
        DefaultSandboxManager sandbox;
        sandbox.setDefaultSandboxMode(SandboxMode::WorkspaceWrite);
        sandbox.clearPaths();
        sandbox.addAllowedReadPath(QStringLiteral("/tmp"));

        QVERIFY(sandbox.canAccess(QStringLiteral("/tmp/test.txt"), FileSystemAccessMode::Read));
        QVERIFY(!sandbox.canAccess(QStringLiteral("/tmp/test.txt"), FileSystemAccessMode::Write));

        sandbox.addAllowedWritePath(QStringLiteral("/tmp"));
        QVERIFY(sandbox.canAccess(QStringLiteral("/tmp/test.txt"), FileSystemAccessMode::Write));

        sandbox.setReadOnlyMode(true);
        QVERIFY(!sandbox.canAccess(QStringLiteral("/tmp/test.txt"), FileSystemAccessMode::Write));
    }
};

QTEST_MAIN(TestExecutionPolicy)
#include "tst_ExecutionPolicy.moc"
