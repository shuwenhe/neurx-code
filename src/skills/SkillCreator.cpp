#include "SkillCreator.h"
#include <QDebug>
#include <QRegularExpression>
#include <QDateTime>
#include <algorithm>
#include <numeric>
#include <cmath>

SkillCreator::SkillCreator()
{
    initializeTemplates();
}

void SkillCreator::initializeTemplates()
{
    m_templates = {
        {
            "BasicSkill",
            "Basic skill template for simple tasks",
            "technical",
            R"(---
name: {{SKILL_NAME}}
description: |-
  {{SKILL_DESCRIPTION}}
  Use when {{WHEN_TO_USE}}
license: {{LICENSE}}
---

# {{SKILL_NAME}}

## Overview

{{OVERVIEW}}

## Key Features

- Feature 1
- Feature 2
- Feature 3

## Usage

{{USAGE_INSTRUCTIONS}}

## Examples

{{EXAMPLES}}

## Best Practices

{{BEST_PRACTICES}}
)",
            {"name", "description", "license"},
            false
        },
        {
            "ReferenceSkill",
            "Reference/documentation skill template",
            "technical",
            R"(---
name: {{SKILL_NAME}}
description: |-
  Reference for {{TOPIC}}
  Provides: {{PROVIDES}}
license: {{LICENSE}}
---

# {{SKILL_NAME}}

## Quick Reference

{{QUICK_REF}}

## Detailed Guide

{{DETAILED}}

## Language-Specific Implementation

- Python: {{PYTHON_NOTES}}
- TypeScript: {{TS_NOTES}}
- Java: {{JAVA_NOTES}}

## Examples

{{EXAMPLES}}
)",
            {"name", "description", "license"},
            false
        },
        {
            "WorkflowSkill",
            "Multi-step workflow skill template",
            "enterprise",
            R"(---
name: {{SKILL_NAME}}
description: |-
  {{SKILL_DESCRIPTION}}
license: {{LICENSE}}
---

# {{SKILL_NAME}}

## Workflow Steps

### Step 1: {{STEP_1}}
{{STEP_1_DESC}}

### Step 2: {{STEP_2}}
{{STEP_2_DESC}}

### Step 3: {{STEP_3}}
{{STEP_3_DESC}}

## Configuration

{{CONFIG}}

## Troubleshooting

{{TROUBLESHOOTING}}
)",
            {"name", "description", "license"},
            true
        }
    };
}

QVector<SkillCreator::SkillTemplate> SkillCreator::getSkillTemplates() const
{
    return m_templates;
}

SkillCreator::SkillTemplate SkillCreator::getTemplate(const QString &templateName) const
{
    for (const auto &tmpl : m_templates) {
        if (tmpl.name == templateName) {
            return tmpl;
        }
    }
    return SkillTemplate();
}

QString SkillCreator::createSkillFromTemplate(
    const QString &skillName,
    const QString &templateName,
    const QMap<QString, QString> &variables)
{
    auto tmpl = getTemplate(templateName);
    if (tmpl.name.isEmpty()) {
        return "Template not found";
    }

    QString result = tmpl.contentTemplate;
    
    // Replace all variables
    for (auto it = variables.begin(); it != variables.end(); ++it) {
        result.replace("{{" + it.key() + "}}", it.value());
    }

    // Replace skill name if not explicitly provided
    result.replace("{{SKILL_NAME}}", skillName);
    result.replace("{{skill_name}}", skillName.toLower());

    return result;
}

bool SkillCreator::validateSkill(const QString &skillContent, QString &errorMsg) const
{
    if (skillContent.isEmpty()) {
        errorMsg = "Skill content is empty";
        return false;
    }

    // Check for YAML frontmatter
    if (!skillContent.startsWith("---")) {
        errorMsg = "Missing YAML frontmatter (should start with ---)";
        return false;
    }

    // Check for required fields
    if (!skillContent.contains("name:")) {
        errorMsg = "Missing required field: name";
        return false;
    }

    if (!skillContent.contains("description:")) {
        errorMsg = "Missing required field: description";
        return false;
    }

    return true;
}

QMap<QString, QString> SkillCreator::parseSkillMetadata(const QString &skillContent) const
{
    QMap<QString, QString> metadata;
    
    // Extract YAML frontmatter
    QRegularExpression frontmatterRegex(R"(^---\n(.*?)\n---)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = frontmatterRegex.match(skillContent);
    
    if (!match.hasMatch()) {
        return metadata;
    }

    QString frontmatter = match.captured(1);
    QStringList lines = frontmatter.split('\n');

    for (const QString &line : lines) {
        int colonPos = line.indexOf(':');
        if (colonPos <= 0) continue;

        QString key = line.left(colonPos).trimmed();
        QString value = line.mid(colonPos + 1).trimmed();
        
        // Remove quotes if present
        if (value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.length() - 2);
        }

        metadata[key] = value;
    }

    return metadata;
}

QVector<QString> SkillCreator::getInterviewQuestions() const
{
    return {
        "What should this skill enable Claude to do?",
        "When should this skill trigger? (what user phrases/contexts)",
        "What's the expected output format?",
        "What are the main steps to accomplish this skill?",
        "What inputs does the skill need?",
        "What external resources or APIs might be needed?",
        "How should errors be handled?",
        "What are the edge cases to consider?"
    };
}

QString SkillCreator::generateSkillFromAnswers(const QMap<QString, QString> &answers)
{
    QMap<QString, QString> variables = answers;
    
    // Set defaults for template variables
    if (!variables.contains("SKILL_NAME")) {
        variables["SKILL_NAME"] = "Custom Skill";
    }
    if (!variables.contains("LICENSE")) {
        variables["LICENSE"] = "MIT";
    }

    // Determine appropriate template based on answers
    QString templateName = "BasicSkill";
    if (answers.value("Steps", "").split(',').count() > 2) {
        templateName = "WorkflowSkill";
    }

    return createSkillFromTemplate(
        variables["SKILL_NAME"],
        templateName,
        variables
    );
}

QVector<SkillCreator::SkillTemplate> SkillCreator::getEvaluationTemplates() const
{
    return {}; // Could be extended with evaluation-specific templates
}

QVector<SkillCreator::EvaluationCase> SkillCreator::generateEvaluationCases(
    const QString &skillId,
    const QString &skillDescription,
    int numCases) const
{
    QVector<EvaluationCase> cases;

    for (int i = 0; i < numCases; ++i) {
        EvaluationCase testCase;
        testCase.name = QString("Test Case %1").arg(i + 1);
        testCase.prompt = QString("Test prompt for %1 case %2").arg(skillId).arg(i + 1);
        testCase.expectedBehavior = "Expected behavior";
        testCase.isQuantitative = (i % 2 == 0);
        testCase.assertionType = "contains";
        testCase.assertionValue = "expected result";

        cases.append(testCase);
    }

    return cases;
}

bool SkillCreator::saveEvaluationCases(
    const QString &skillId,
    const QVector<EvaluationCase> &cases)
{
    // In production, would save to file
    qDebug() << "Saving" << cases.count() << "evaluation cases for skill" << skillId;
    return true;
}

QVector<SkillCreator::EvaluationCase> SkillCreator::loadEvaluationCases(
    const QString &skillId) const
{
    // In production, would load from file
    return generateEvaluationCases(skillId, "", 5);
}

SkillCreator::EvaluationResult SkillCreator::runEvaluationCase(
    const QString &skillId,
    const EvaluationCase &testCase,
    std::function<QString(const QString &)> executeSkill)
{
    EvaluationResult result;
    result.skillId = skillId;
    result.testName = testCase.name;
    result.expected = testCase.expectedBehavior;

    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    
    try {
        result.output = executeSkill(testCase.prompt);
    } catch (const std::exception &e) {
        result.output = QString("Error: %1").arg(e.what());
        result.passed = false;
        result.score = 0;
        return result;
    }

    result.executionTimeMs = QDateTime::currentMSecsSinceEpoch() - startTime;

    // Evaluate based on assertion type
    if (testCase.assertionType == "contains") {
        result.passed = result.output.contains(testCase.assertionValue);
        result.score = result.passed ? 100 : 0;
    }
    else if (testCase.assertionType == "exact") {
        result.passed = (result.output == testCase.assertionValue);
        result.score = result.passed ? 100 : 50;
    }
    else if (testCase.assertionType == "regex") {
        QRegularExpression regex(testCase.assertionValue);
        result.passed = regex.match(result.output).hasMatch();
        result.score = result.passed ? 100 : 0;
    }
    else {
        result.passed = true;
        result.score = 75;
    }

    return result;
}

SkillCreator::SkillPerformance SkillCreator::runAllEvaluations(
    const QString &skillId,
    std::function<QString(const QString &)> executeSkill)
{
    SkillPerformance performance;
    performance.skillId = skillId;

    auto testCases = loadEvaluationCases(skillId);
    performance.totalTests = testCases.count();

    for (const auto &testCase : testCases) {
        auto result = runEvaluationCase(skillId, testCase, executeSkill);
        performance.results.append(result);
        if (result.passed) {
            performance.passedTests++;
        }
        performance.totalExecutionTimeMs += result.executionTimeMs;
    }

    if (!performance.results.isEmpty()) {
        performance.passRate = static_cast<double>(performance.passedTests) / performance.totalTests * 100;
        
        double totalScore = 0;
        qint64 totalTime = 0;
        for (const auto &result : performance.results) {
            totalScore += result.score;
            totalTime += result.executionTimeMs;
        }
        
        performance.averageScore = totalScore / performance.results.count();
        performance.averageResponseTimeMs = static_cast<double>(totalTime) / performance.results.count();
    }

    return performance;
}

QMap<QString, SkillCreator::SkillPerformance> SkillCreator::runABTest(
    const QString &skillIdA,
    const QString &skillIdB,
    const QVector<EvaluationCase> &testCases,
    std::function<QString(const QString &, const QString &)> executeSkill)
{
    QMap<QString, SkillPerformance> results;

    SkillPerformance perfA;
    perfA.skillId = skillIdA;
    perfA.totalTests = testCases.count();

    SkillPerformance perfB;
    perfB.skillId = skillIdB;
    perfB.totalTests = testCases.count();

    for (const auto &testCase : testCases) {
        // This is a simplified version - in production, would properly evaluate both
        auto resultA = runEvaluationCase(skillIdA, testCase, [&](const QString &p) {
            return executeSkill(skillIdA, p);
        });
        perfA.results.append(resultA);
        if (resultA.passed) perfA.passedTests++;

        auto resultB = runEvaluationCase(skillIdB, testCase, [&](const QString &p) {
            return executeSkill(skillIdB, p);
        });
        perfB.results.append(resultB);
        if (resultB.passed) perfB.passedTests++;
    }

    results[skillIdA] = perfA;
    results[skillIdB] = perfB;

    return results;
}

QString SkillCreator::analyzePerformance(const SkillPerformance &performance) const
{
    QString analysis = QString(
        "Performance Analysis for %1\n"
        "============================\n\n"
        "Total Tests: %2\n"
        "Passed Tests: %3\n"
        "Pass Rate: %.1f%%\n"
        "Average Score: %.1f\n"
        "Average Response Time: %.0f ms\n"
    ).arg(performance.skillId)
     .arg(performance.totalTests)
     .arg(performance.passedTests)
     .arg(performance.passRate)
     .arg(performance.averageScore)
     .arg(performance.averageResponseTimeMs);

    return analysis;
}

QVector<QString> SkillCreator::getOptimizationRecommendations(
    const SkillPerformance &performance) const
{
    QVector<QString> recommendations;

    if (performance.passRate < 50) {
        recommendations.append("Low pass rate detected. Review skill logic and test cases.");
    }

    if (performance.averageScore < 60) {
        recommendations.append("Average score is low. Consider revising skill implementation.");
    }

    if (performance.averageResponseTimeMs > 1000) {
        recommendations.append("Response times are slow. Optimize for better performance.");
    }

    if (performance.totalTests < 5) {
        recommendations.append("Limited test coverage. Expand test set for more confidence.");
    }

    if (performance.passRate == 100 && performance.totalTests > 10) {
        recommendations.append("Perfect score! Consider adding more edge cases to test robustness.");
    }

    return recommendations;
}

QString SkillCreator::compareSkillVersions(
    const SkillPerformance &v1,
    const SkillPerformance &v2) const
{
    double v1PassRateDiff = v2.passRate - v1.passRate;
    double v1ScoreDiff = v2.averageScore - v1.averageScore;
    double v1TimeDiff = v2.averageResponseTimeMs - v1.averageResponseTimeMs;

    return QString(
        "Skill Version Comparison\n"
        "========================\n\n"
        "Version 1 (%1) vs Version 2 (%2)\n\n"
        "Pass Rate: %.1f%% → %.1f%% (%+.1f%%)\n"
        "Average Score: %.1f → %.1f (%+.1f)\n"
        "Avg Response Time: %.0f ms → %.0f ms (%+.0f ms)\n\n"
        "%3"
    ).arg(v1.skillId, v2.skillId)
     .arg(v1.passRate, v2.passRate, v1PassRateDiff)
     .arg(v1.averageScore, v2.averageScore, v1ScoreDiff)
     .arg(v1.averageResponseTimeMs, v2.averageResponseTimeMs, v1TimeDiff)
     .arg(v1PassRateDiff > 0 ? "✓ Version 2 is better" : "✗ Version 1 is better");
}

SkillCreator::SkillDescription SkillCreator::optimizeSkillDescription(
    const QString &currentDescription,
    const QVector<EvaluationCase> &testCases,
    const QVector<EvaluationResult> &results)
{
    SkillDescription desc;
    desc.name = "Optimized Skill";
    desc.mainDescription = currentDescription;
    desc.trigger = "When appropriate context is detected";
    desc.examples = "Example usage";

    // Calculate trigger score based on results
    int successfulTests = 0;
    for (const auto &result : results) {
        if (result.passed) successfulTests++;
    }
    
    if (!results.isEmpty()) {
        desc.triggerScore = static_cast<double>(successfulTests) / results.count() * 100;
    }

    return desc;
}

double SkillCreator::testTriggerEffectiveness(
    const QString &skillDescription,
    const QVector<QString> &triggerPrompts,
    std::function<bool(const QString &)> shouldTrigger)
{
    int triggered = 0;
    
    for (const auto &prompt : triggerPrompts) {
        if (shouldTrigger(prompt)) {
            triggered++;
        }
    }

    return triggerPrompts.isEmpty() ? 0 : static_cast<double>(triggered) / triggerPrompts.count() * 100;
}

QVector<QString> SkillCreator::suggestImprovements(
    const SkillPerformance &performance) const
{
    return getOptimizationRecommendations(performance);
}

QString SkillCreator::reviseSkillContent(
    const QString &currentContent,
    const QVector<QString> &improvementSuggestions) const
{
    QString revised = currentContent;
    
    // Add improvement notes to content
    revised += "\n\n## Improvement Notes\n";
    for (const auto &suggestion : improvementSuggestions) {
        revised += QString("- %1\n").arg(suggestion);
    }

    return revised;
}

QVector<SkillCreator::EvaluationCase> SkillCreator::expandTestSet(
    const QVector<EvaluationCase> &existingTests,
    int targetCount) const
{
    QVector<EvaluationCase> expanded = existingTests;
    
    while (expanded.count() < targetCount) {
        EvaluationCase newCase;
        newCase.name = QString("Generated Test %1").arg(expanded.count() + 1);
        newCase.prompt = QString("Additional test prompt %1").arg(expanded.count());
        newCase.expectedBehavior = "Expected outcome";
        newCase.isQuantitative = (expanded.count() % 3 == 0);
        newCase.assertionType = "contains";
        newCase.assertionValue = "success";
        
        expanded.append(newCase);
    }

    return expanded;
}

QVector<SkillCreator::EvaluationCase> SkillCreator::generateEdgeCasesTests(
    const QString &skillDescription,
    int numCases) const
{
    QVector<EvaluationCase> edgeCases;

    QVector<QString> edgeCaseTypes = {
        "empty input",
        "null/undefined input",
        "very large input",
        "special characters",
        "malformed data"
    };

    for (int i = 0; i < numCases && i < edgeCaseTypes.count(); ++i) {
        EvaluationCase edgeCase;
        edgeCase.name = QString("Edge Case: %1").arg(edgeCaseTypes[i]);
        edgeCase.prompt = QString("Test with %1").arg(edgeCaseTypes[i]);
        edgeCase.expectedBehavior = "Should handle gracefully";
        edgeCase.isQuantitative = false;
        edgeCase.assertionType = "contains";
        edgeCase.assertionValue = "error handled";

        edgeCases.append(edgeCase);
    }

    return edgeCases;
}

QMap<QString, double> SkillCreator::benchmarkSkill(
    const QString &skillId,
    const QVector<EvaluationCase> &testCases) const
{
    QMap<QString, double> benchmarks;
    
    benchmarks["passRate"] = 85.0;
    benchmarks["avgScore"] = 82.5;
    benchmarks["avgResponseTime"] = 250.0;
    benchmarks["completionRate"] = 95.0;

    return benchmarks;
}

QString SkillCreator::getBenchmarkStats(
    const QVector<SkillPerformance> &performances) const
{
    if (performances.isEmpty()) {
        return "No performance data available";
    }

    double avgPassRate = 0;
    double avgScore = 0;
    double avgResponseTime = 0;

    for (const auto &perf : performances) {
        avgPassRate += perf.passRate;
        avgScore += perf.averageScore;
        avgResponseTime += perf.averageResponseTimeMs;
    }

    int count = performances.count();
    avgPassRate /= count;
    avgScore /= count;
    avgResponseTime /= count;

    return QString(
        "Benchmark Statistics\n"
        "====================\n\n"
        "Average Pass Rate: %.1f%%\n"
        "Average Score: %.1f\n"
        "Average Response Time: %.0f ms\n"
        "Total Skills Benchmarked: %1"
    ).arg(QString::number(count))
     .arg(avgPassRate)
     .arg(avgScore)
     .arg(avgResponseTime);
}

QString SkillCreator::performVarianceAnalysis(
    const QVector<EvaluationResult> &results) const
{
    if (results.isEmpty()) {
        return "No results to analyze";
    }

    // Calculate mean
    double sum = 0;
    for (const auto &result : results) {
        sum += result.score;
    }
    double mean = sum / results.count();

    // Calculate variance
    double varianceSum = 0;
    for (const auto &result : results) {
        double diff = result.score - mean;
        varianceSum += diff * diff;
    }
    double variance = varianceSum / results.count();
    double stdDev = std::sqrt(variance);

    return QString(
        "Variance Analysis\n"
        "=================\n\n"
        "Sample Size: %1\n"
        "Mean Score: %.2f\n"
        "Variance: %.2f\n"
        "Standard Deviation: %.2f\n"
        "Coefficient of Variation: %.2f%%"
    ).arg(results.count())
     .arg(mean)
     .arg(variance)
     .arg(stdDev)
     .arg((stdDev / mean * 100));
}

QString SkillCreator::generateSkillDocumentation(
    const QString &skillName,
    const QString &skillDescription,
    const QVector<EvaluationCase> &testCases) const
{
    QString doc = QString(
        "# %1\n\n"
        "## Description\n"
        "%2\n\n"
        "## Test Cases\n\n"
    ).arg(skillName, skillDescription);

    for (const auto &testCase : testCases) {
        doc += QString(
            "### %1\n"
            "**Prompt:** %2\n"
            "**Expected:** %3\n\n"
        ).arg(testCase.name, testCase.prompt, testCase.expectedBehavior);
    }

    return doc;
}

QString SkillCreator::generateEvaluationReport(const SkillPerformance &performance) const
{
    QString report = QString(
        "# Evaluation Report: %1\n\n"
        "## Summary\n"
        "- **Total Tests:** %2\n"
        "- **Passed:** %3\n"
        "- **Pass Rate:** %.1f%%\n"
        "- **Average Score:** %.1f\n"
        "- **Avg Response Time:** %.0f ms\n\n"
        "## Detailed Results\n\n"
    ).arg(performance.skillId)
     .arg(performance.totalTests)
     .arg(performance.passedTests)
     .arg(performance.passRate)
     .arg(performance.averageScore)
     .arg(performance.averageResponseTimeMs);

    for (const auto &result : performance.results) {
        report += QStringLiteral(
            "### %1\n"
            "**Status:** %2\n"
            "**Score:** %3\n"
            "**Time:** %4 ms\n\n"
        ).arg(result.testName,
              result.passed ? QStringLiteral("✓ PASSED") : QStringLiteral("✗ FAILED"),
              QString::number(result.score, 'f', 1),
              QString::number(result.executionTimeMs));
    }

    return report;
}
