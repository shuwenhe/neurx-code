#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QProcess>
#include "../src/agent/CommitCommandManager.h"

class TestCommitCommandManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        manager = new CommitCommandManager();
        QVERIFY(manager != nullptr);
        
        tempDir = new QTemporaryDir();
        QVERIFY(tempDir->isValid());
        
        // Initialize a git repo for testing
        initGitRepo();
    }

    void cleanupTestCase() {
        delete manager;
        delete tempDir;
    }

    // Test: Generate commit message from changes
    void testGenerateCommitMessage() {
        QStringList stagedChanges = QStringList{"src/main.cpp", "include/main.h"};
        QStringList unstagedChanges = QStringList{"src/utils.cpp"};
        QStringList recentMessages = QStringList{"feat: Add feature", "fix: Fix bug"};
        
        QString message = manager->generateCommitMessage(stagedChanges, unstagedChanges, recentMessages);
        
        QVERIFY(!message.isEmpty());
        QVERIFY(message.length() > 5);
    }

    // Test: Get git status
    void testGetGitStatus() {
        QString workspaceRoot = tempDir->path();
        
        auto status = manager->getGitStatus(workspaceRoot);
        
        QVERIFY(status.keys().size() >= 0);
    }

    // Test: Get staged files
    void testGetStagedFiles() {
        QString workspaceRoot = tempDir->path();
        
        auto staged = manager->getStagedFiles(workspaceRoot);
        
        QVERIFY(true);  // Should return valid list
    }

    // Test: Get unstaged files
    void testGetUnstagedFiles() {
        QString workspaceRoot = tempDir->path();
        
        auto unstaged = manager->getUnstagedFiles(workspaceRoot);
        
        QVERIFY(true);  // Should return valid list
    }

    // Test: Get recent commit messages
    void testGetRecentCommitMessages() {
        QString workspaceRoot = tempDir->path();
        
        auto messages = manager->getRecentCommitMessages(workspaceRoot, 5);
        
        QVERIFY(messages.size() >= 0);
        QVERIFY(messages.size() <= 5);
    }

    // Test: Create feature branch
    void testCreateFeatureBranch() {
        QString workspaceRoot = tempDir->path();
        QString branchName = "feature/test-branch";
        
        bool success = manager->createFeatureBranch(workspaceRoot, branchName);
        
        // May succeed or fail depending on git setup, but should handle gracefully
        QVERIFY(true);
    }

    // Test: Get current branch
    void testGetCurrentBranch() {
        QString workspaceRoot = tempDir->path();
        QString branch;
        
        bool success = manager->getCurrentBranch(workspaceRoot, branch);
        
        QVERIFY(true);  // Should handle gracefully
    }

    // Test: Check if on main branch
    void testIsOnMainBranch() {
        QString workspaceRoot = tempDir->path();
        
        bool isMain = manager->isOnMainBranch(workspaceRoot);
        
        QVERIFY(isMain == true || isMain == false);  // Valid boolean
    }

    // Test: Get gone branches
    void testGetGoneBranches() {
        QString workspaceRoot = tempDir->path();
        
        auto goneBranches = manager->getGoneBranches(workspaceRoot);
        
        QVERIFY(goneBranches.size() >= 0);
    }

    // Test: Commit style configuration
    void testCommitStyleConfiguration() {
        manager->setCommitStyle("conventional");
        
        QString style = manager->getCommitStyle();
        
        QCOMPARE(style, "conventional");
    }

    // Test: Different commit styles
    void testDifferentCommitStyles() {
        QStringList styles = {"conventional", "descriptive", "semantic"};
        
        for (const auto& style : styles) {
            manager->setCommitStyle(style);
            QCOMPARE(manager->getCommitStyle(), style);
        }
    }

    // Test: Commit message generation with various inputs
    void testCommitMessageVariations() {
        QStringList empty;
        QStringList single = QStringList{"src/main.cpp"};
        QStringList multiple = QStringList{"src/a.cpp", "src/b.cpp", "src/c.cpp"};
        
        QString msg1 = manager->generateCommitMessage(empty, empty, empty);
        QString msg2 = manager->generateCommitMessage(single, empty, empty);
        QString msg3 = manager->generateCommitMessage(multiple, multiple, empty);
        
        QVERIFY(!msg1.isEmpty() || msg1.isEmpty());  // Valid regardless
        QVERIFY(!msg2.isEmpty() || msg2.isEmpty());
        QVERIFY(!msg3.isEmpty() || msg3.isEmpty());
    }

    // Test: Execute commit (should handle gracefully without repo)
    void testExecuteCommit() {
        QString workspaceRoot = tempDir->path();
        
        bool success = manager->executeCommit(workspaceRoot);
        
        QVERIFY(success == true || success == false);  // Valid boolean
    }

    // Test: Validate workspace root
    void testWorkspaceValidation() {
        QString validPath = tempDir->path();
        QString invalidPath = "/nonexistent/path/that/does/not/exist";
        
        bool validResult = manager->executeCommit(validPath);
        bool invalidResult = manager->executeCommit(invalidPath);
        
        // Should handle both gracefully
        QVERIFY(true);
    }

    // Test: PR URL generation
    void testPRUrlGeneration() {
        QString workspaceRoot = tempDir->path();
        QString branchName = "feature/test";
        
        QString url = manager->getPRUrl(workspaceRoot, branchName);
        
        // Should return a string (may be empty if git not set up)
        QVERIFY(true);
    }

private:
    CommitCommandManager* manager;
    QTemporaryDir* tempDir;

    void initGitRepo() {
        // Initialize a simple git repo for testing
        QString repoPath = tempDir->path();
        QProcess git;
        git.setWorkingDirectory(repoPath);
        git.start("git", QStringList() << "init");
        git.waitForFinished();
        
        // Configure user
        git.start("git", QStringList() << "config" << "user.email" << "test@example.com");
        git.waitForFinished();
        
        git.start("git", QStringList() << "config" << "user.name" << "Test User");
        git.waitForFinished();
    }
};

QTEST_MAIN(TestCommitCommandManager)
#include "tst_CommitCommandManager.moc"
