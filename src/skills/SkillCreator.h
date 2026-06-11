#pragma once

#include <QString>
#include <QMap>
#include <QVector>
#include <memory>
#include <functional>

/**
 * @class SkillCreator
 * @brief Skill Creator and Optimizer
 * 
 * Provides guidance and tools for:
 * - Creating new skills from scratch
 * - Modifying and improving existing skills
 * - Running evaluations to test skill performance
 * - Measuring and benchmarking skill effectiveness
 */
class SkillCreator {
public:
    struct SkillTemplate {
        QString name;
        QString description;
        QString category;  // "creative", "technical", "enterprise", "document"
        QString contentTemplate;  // Markdown content
        QVector<QString> requiredSections;
        bool hasTestCases;
    };

    struct EvaluationCase {
        QString name;
        QString prompt;
        QString expectedBehavior;
        bool isQuantitative;
        QString assertionType;  // "contains", "exact", "regex", "function"
        QString assertionValue;
    };

    struct EvaluationResult {
        QString skillId;
        QString testName;
        bool passed;
        QString output;
        QString expected;
        double score;  // 0-100
        qint64 executionTimeMs;
    };

    struct SkillPerformance {
        QString skillId;
        int totalTests;
        int passedTests;
        double passRate;
        double averageScore;
        qint64 totalExecutionTimeMs;
        double averageResponseTimeMs;
        QVector<EvaluationResult> results;
    };

    struct SkillDescription {
        QString name;
        QString trigger;        // When should this skill be used?
        QString mainDescription;
        QString examples;
        QVector<QString> keywords;
        double triggerScore;    // How likely is skill to trigger correctly?
    };

    SkillCreator();
    virtual ~SkillCreator() = default;

    // ── Skill Creation ────────────────────────────────────

    /// Get available skill templates
    virtual QVector<SkillTemplate> getSkillTemplates() const;

    /// Get specific template
    virtual SkillTemplate getTemplate(const QString &templateName) const;

    /// Create new skill from template
    virtual QString createSkillFromTemplate(
        const QString &skillName,
        const QString &templateName,
        const QMap<QString, QString> &variables
    );

    /// Validate skill structure
    virtual bool validateSkill(const QString &skillContent, QString &errorMsg) const;

    /// Parse skill metadata from SKILL.md
    virtual QMap<QString, QString> parseSkillMetadata(const QString &skillContent) const;

    // ── Skill Interview ──────────────────────────────────

    /// Interview questions for skill creation
    virtual QVector<QString> getInterviewQuestions() const;

    /// Generate skill from interview answers
    virtual QString generateSkillFromAnswers(
        const QMap<QString, QString> &answers
    );

    // ── Evaluation Setup ─────────────────────────────────

    /// Get evaluation templates
    virtual QVector<SkillTemplate> getEvaluationTemplates() const;

    /// Create evaluation cases from skill description
    virtual QVector<EvaluationCase> generateEvaluationCases(
        const QString &skillId,
        const QString &skillDescription,
        int numCases = 5
    ) const;

    /// Save evaluation cases
    virtual bool saveEvaluationCases(
        const QString &skillId,
        const QVector<EvaluationCase> &cases
    );

    /// Load evaluation cases
    virtual QVector<EvaluationCase> loadEvaluationCases(const QString &skillId) const;

    // ── Evaluation Execution ──────────────────────────────

    /// Run single evaluation case
    virtual EvaluationResult runEvaluationCase(
        const QString &skillId,
        const EvaluationCase &testCase,
        std::function<QString(const QString &)> executeSkill
    );

    /// Run all evaluations for skill
    virtual SkillPerformance runAllEvaluations(
        const QString &skillId,
        std::function<QString(const QString &)> executeSkill
    );

    /// Run A/B test between two skill versions
    virtual QMap<QString, SkillPerformance> runABTest(
        const QString &skillIdA,
        const QString &skillIdB,
        const QVector<EvaluationCase> &testCases,
        std::function<QString(const QString &, const QString &)> executeSkill
    );

    // ── Performance Analysis ──────────────────────────────

    /// Analyze performance metrics
    virtual QString analyzePerformance(const SkillPerformance &performance) const;

    /// Get performance recommendations
    virtual QVector<QString> getOptimizationRecommendations(
        const SkillPerformance &performance
    ) const;

    /// Compare skill versions
    virtual QString compareSkillVersions(
        const SkillPerformance &v1,
        const SkillPerformance &v2
    ) const;

    // ── Trigger Optimization ──────────────────────────────

    /// Optimize skill description for better triggering
    virtual SkillDescription optimizeSkillDescription(
        const QString &currentDescription,
        const QVector<EvaluationCase> &testCases,
        const QVector<EvaluationResult> &results
    );

    /// Test trigger effectiveness
    virtual double testTriggerEffectiveness(
        const QString &skillDescription,
        const QVector<QString> &triggerPrompts,
        std::function<bool(const QString &)> shouldTrigger
    );

    // ── Iterative Improvement ────────────────────────────

    /// Suggest improvements based on evaluation results
    virtual QVector<QString> suggestImprovements(const SkillPerformance &performance) const;

    /// Generate revised skill content
    virtual QString reviseSkillContent(
        const QString &currentContent,
        const QVector<QString> &improvementSuggestions
    ) const;

    // ── Test Set Expansion ────────────────────────────────

    /// Expand test cases for broader coverage
    virtual QVector<EvaluationCase> expandTestSet(
        const QVector<EvaluationCase> &existingTests,
        int targetCount = 20
    ) const;

    /// Generate edge case tests
    virtual QVector<EvaluationCase> generateEdgeCasesTests(
        const QString &skillDescription,
        int numCases = 5
    ) const;

    // ── Benchmarking ──────────────────────────────────────

    /// Benchmark skill against reference implementations
    virtual QMap<QString, double> benchmarkSkill(
        const QString &skillId,
        const QVector<EvaluationCase> &testCases
    ) const;

    /// Get benchmark statistics
    virtual QString getBenchmarkStats(
        const QVector<SkillPerformance> &performances
    ) const;

    /// Variance analysis for results
    virtual QString performVarianceAnalysis(
        const QVector<EvaluationResult> &results
    ) const;

    // ── Documentation ────────────────────────────────────

    /// Generate skill documentation
    virtual QString generateSkillDocumentation(
        const QString &skillName,
        const QString &skillDescription,
        const QVector<EvaluationCase> &testCases
    ) const;

    /// Generate evaluation report
    virtual QString generateEvaluationReport(const SkillPerformance &performance) const;

private:
    void initializeTemplates();
    QVector<SkillTemplate> m_templates;
};

using SkillCreatorPtr = std::shared_ptr<SkillCreator>;
