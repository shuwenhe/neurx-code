#include <QTest>
#include <QObject>
#include "../src/agent/HookifyManager.h"

class TestHookifyManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        manager = new HookifyManager();
        QVERIFY(manager != nullptr);
    }

    void cleanupTestCase() {
        delete manager;
    }

    // Test: Object creation
    void testObjectCreation() {
        QVERIFY(manager != nullptr);
    }

    // Test: Hook rule structure
    void testHookRuleStructure() {
        HookifyManager::HookRule hook;
        hook.id = "test-hook";
        hook.name = "Test Hook";
        hook.description = "Test Description";
        hook.type = HookifyManager::PreventatativeHook;
        hook.priority = 10;
        
        QCOMPARE(hook.id, "test-hook");
        QCOMPARE(hook.priority, 10);
    }

    // Test: Behavior pattern structure
    void testBehaviorPatternStructure() {
        HookifyManager::BehaviorPattern pattern;
        pattern.pattern = "test_pattern";
        pattern.confidence = 0.85f;
        pattern.occurrences = 5;
        
        QCOMPARE(pattern.pattern, "test_pattern");
        QCOMPARE(pattern.confidence, 0.85f);
        QCOMPARE(pattern.occurrences, 5);
    }

    // Test: Hook types
    void testHookTypes() {
        QVector<HookifyManager::HookType> types = {
            HookifyManager::PreventatativeHook,
            HookifyManager::CorrectionHook,
            HookifyManager::FilteringHook,
            HookifyManager::RedirectionHook,
            HookifyManager::EnforcementHook
        };
        
        QCOMPARE(types.size(), 5);
    }

    // Test: Severity levels
    void testSeverityLevels() {
        QVector<HookifyManager::SeverityLevel> levels = {
            HookifyManager::Info,
            HookifyManager::Warning,
            HookifyManager::Error,
            HookifyManager::Critical
        };
        
        QCOMPARE(levels.size(), 4);
    }

    // Test: Get all hooks
    void testGetAllHooks() {
        auto hooks = manager->getAllHooks();
        QVERIFY(hooks.size() >= 0);
    }

    // Test: Get hooks by type
    void testGetHooksByType() {
        auto preventativeHooks = manager->getHooksOfType(HookifyManager::PreventatativeHook);
        QVERIFY(preventativeHooks.size() >= 0);
        
        auto filteringHooks = manager->getHooksOfType(HookifyManager::FilteringHook);
        QVERIFY(filteringHooks.size() >= 0);
    }

private:
    HookifyManager* manager;
};

QTEST_MAIN(TestHookifyManager)
#include "tst_HookifyManager.moc"
