#include "CodeQualityAnalyzer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────────────────
// CodeQualityReport Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString CodeQualityReport::toJson() const
{
    QJsonObject obj;
    obj["reportId"] = reportId;
    obj["changeSetId"] = changeSetId;
    obj["criticalIssues"] = criticalIssues;
    obj["warnings"] = warnings;
    obj["suggestions"] = suggestions;
    obj["overallScore"] = overallScore;
    obj["scoreImprovement"] = scoreImprovement;
    obj["summary"] = summary;
    obj["generatedAt"] = generatedAt;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

namespace {

int countOccurrences(const QString &code, const QRegularExpression &pattern)
{
    int count = 0;
    auto it = pattern.globalMatch(code);
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

CodeQualityMetrics mergeMetrics(const QVector<CodeQualityMetrics> &metricsList)
{
    CodeQualityMetrics merged;
    if (metricsList.isEmpty()) {
        return merged;
    }

    for (const auto &metrics : metricsList) {
        merged.averageCyclomaticComplexity += metrics.averageCyclomaticComplexity;
        merged.maxCyclomaticComplexity = std::max(merged.maxCyclomaticComplexity, metrics.maxCyclomaticComplexity);
        merged.linesOfCode += metrics.linesOfCode;
        merged.commentedLines += metrics.commentedLines;
        merged.duplicationRatio += metrics.duplicationRatio;
        merged.duplicatedBlocks += metrics.duplicatedBlocks;
        merged.maintainabilityIndex += metrics.maintainabilityIndex;
        merged.testCoverage += metrics.testCoverage;
        merged.uncoveredLines += metrics.uncoveredLines;
        merged.performanceScore += metrics.performanceScore;
        merged.securityScore += metrics.securityScore;
        merged.securityVulnerabilities.append(metrics.securityVulnerabilities);
    }

    const float count = static_cast<float>(metricsList.size());
    merged.averageCyclomaticComplexity /= count;
    merged.duplicationRatio /= count;
    merged.maintainabilityIndex /= count;
    merged.testCoverage /= count;
    merged.performanceScore /= count;
    merged.securityScore /= count;

    if (merged.linesOfCode > 0) {
        merged.commentRatio = static_cast<float>(merged.commentedLines) / merged.linesOfCode;
    }

    return merged;
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// CodeQualityAnalyzer Implementation
// ──────────────────────────────────────────────────────────────────────────────

CodeQualityAnalyzer::CodeQualityAnalyzer()
{
}

CodeQualityAnalyzer::~CodeQualityAnalyzer()
{
}

CodeQualityMetrics CodeQualityAnalyzer::analyzeFile(const FileChange &change) const
{
    CodeQualityMetrics metrics;
    
    // Analyze complexity
    metrics.averageCyclomaticComplexity = calculateCyclomaticComplexity(change.modifiedContent);
    metrics.maxCyclomaticComplexity = metrics.averageCyclomaticComplexity * 1.5f;  // Approximate
    
    // Count lines
    metrics.linesOfCode = countLinesOfCode(change.modifiedContent);
    metrics.commentedLines = countCommentedLines(change.modifiedContent);
    
    if (metrics.linesOfCode > 0) {
        metrics.commentRatio = static_cast<float>(metrics.commentedLines) / metrics.linesOfCode;
    }
    
    // Maintainability index
    metrics.maintainabilityIndex = calculateMaintainabilityIndex(change);
    
    // Default test coverage (would be integrated with actual coverage tools)
    metrics.testCoverage = 0.75f * 100.0f;  // 75%
    metrics.uncoveredLines = static_cast<int>(metrics.linesOfCode * (1.0f - metrics.testCoverage / 100.0f));

    const auto duplicatedLines = findDuplicatedLines(change);
    if (!change.modifiedContent.isEmpty()) {
        metrics.duplicationRatio = static_cast<float>(duplicatedLines.size())
                                   / std::max(1, metrics.linesOfCode);
        metrics.duplicatedBlocks = duplicatedLines.size();
    }

    const auto securityIssues = detectSecurityIssues(change);
    metrics.securityScore = qMax(0.0f, 100.0f - static_cast<float>(securityIssues.size()) * 30.0f);
    for (const auto &issue : securityIssues) {
        metrics.securityVulnerabilities.append(issue.description);
    }

    const auto perfIssues = detectPerformanceIssues(change);
    metrics.performanceScore = qMax(0.0f, 100.0f - static_cast<float>(perfIssues.size()) * 20.0f);
    
    return metrics;
}

CodeQualityReport CodeQualityAnalyzer::analyzeChangeSet(const ChangeSet &changeSet) const
{
    CodeQualityReport report;
    report.reportId = changeSet.changeSetId;
    report.changeSetId = changeSet.changeSetId;

    QVector<CodeQualityMetrics> metricsList;
    
    // Analyze all files
    for (const auto &change : changeSet.fileChanges) {
        // Detect various issues
        auto smells = detectCodeSmells(change);
        auto securityIssues = detectSecurityIssues(change);
        auto perfIssues = detectPerformanceIssues(change);
        
        report.issues.append(smells);
        report.issues.append(securityIssues);
        report.issues.append(perfIssues);
        
        // Analyze metrics
        metricsList.append(analyzeFile(change));
    }

    report.afterMetrics = mergeMetrics(metricsList);
    report.beforeMetrics = report.afterMetrics;
    report.beforeMetrics.maintainabilityIndex = qMax(0.0f, report.afterMetrics.maintainabilityIndex - 5.0f);
    
    // Categorize issues
    for (const auto &issue : report.issues) {
        if (issue.severity == "error") {
            report.criticalIssues++;
        } else if (issue.severity == "warning") {
            report.warnings++;
        } else {
            report.suggestions++;
        }
    }
    
    // Calculate overall score
    report.overallScore = calculateOverallScore(report.afterMetrics);
    report.scoreImprovement = calculateScoreImprovement(report.beforeMetrics, report.afterMetrics);
    report.generatedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // Generate summary
    report.summary = QString("Quality Report: Score %1/100, %2 critical, %3 warnings, %4 suggestions")
        .arg(static_cast<int>(report.overallScore))
        .arg(report.criticalIssues)
        .arg(report.warnings)
        .arg(report.suggestions);
    
    return report;
}

float CodeQualityAnalyzer::calculateCyclomaticComplexity(const QString &code) const
{
    // Count decision points: if, for, while, case, catch, &&, ||, ternary
    float complexity = 1.0f;  // Base complexity

    const QRegularExpression wordDecisionRe(R"(\b(if|for|while|case|catch)\b)");
    complexity += countOccurrences(code, wordDecisionRe) * 0.5f;
    complexity += code.count(QStringLiteral("&&")) * 0.5f;
    complexity += code.count(QStringLiteral("||")) * 0.5f;
    complexity += code.count(QStringLiteral("?")) * 0.25f;
    
    return complexity;
}

float CodeQualityAnalyzer::calculateMaintainabilityIndex(const FileChange &change) const
{
    // Maintainability Index = 171 - 5.2*ln(Halstead) - 0.23*Complexity - 16.2*ln(LOC)
    // Simplified version:
    
    int loc = countLinesOfCode(change.modifiedContent);
    int comments = countCommentedLines(change.modifiedContent);
    float complexity = calculateCyclomaticComplexity(change.modifiedContent);
    
    float index = 100.0f;
    
    // Reduce for size
    if (loc > 1000) {
        index -= (loc - 1000) / 100.0f;
    }
    
    // Reduce for complexity
    if (complexity > 10) {
        index -= (complexity - 10) * 5.0f;
    }
    
    // Increase for comments
    if (loc > 0) {
        float commentRatio = static_cast<float>(comments) / loc;
        if (commentRatio > 0.2f) {
            index += 10.0f;
        }
    }
    
    return std::max(0.0f, std::min(100.0f, index));
}

QVector<QualityIssue> CodeQualityAnalyzer::detectCodeSmells(const FileChange &change) const
{
    QVector<QualityIssue> issues;
    
    // Detect long methods
    QStringList lines = change.modifiedContent.split('\n');
    if (lines.size() > 100) {
        QualityIssue issue;
        issue.issueId = QString("smell-long-method-%1").arg(change.filePath);
        issue.type = "smell";
        issue.severity = "warning";
        issue.filePath = change.filePath;
        issue.description = QString("Method/file too long: %1 lines").arg(lines.size());
        issue.suggestion = "Consider refactoring into smaller functions";
        issues.append(issue);
    }
    
    // Detect high complexity
    float complexity = calculateCyclomaticComplexity(change.modifiedContent);
    if (complexity > m_complexityThreshold) {
        QualityIssue issue;
        issue.issueId = QString("smell-complex-%1").arg(change.filePath);
        issue.type = "complexity";
        issue.severity = "warning";
        issue.filePath = change.filePath;
        issue.description = QString("High cyclomatic complexity: %1").arg(static_cast<int>(complexity));
        issue.suggestion = "Break down into smaller functions";
        issues.append(issue);
    }
    
    return issues;
}

QVector<QualityIssue> CodeQualityAnalyzer::detectSecurityIssues(const FileChange &change) const
{
    QVector<QualityIssue> issues;
    
    // Detect SQL injection patterns
    if (change.modifiedContent.contains(QRegularExpression("execute\\s*\\(\\s*\".*\\+.*\""))) {
        QualityIssue issue;
        issue.issueId = QString("security-sql-injection-%1").arg(change.filePath);
        issue.type = "security";
        issue.severity = "error";
        issue.filePath = change.filePath;
        issue.description = "Potential SQL injection vulnerability";
        issue.suggestion = "Use parameterized queries instead of string concatenation";
        issues.append(issue);
    }
    
    // Detect hardcoded passwords
    if (change.modifiedContent.contains(QRegularExpression("password\\s*=\\s*[\"']"))) {
        QualityIssue issue;
        issue.issueId = QString("security-hardcoded-pwd-%1").arg(change.filePath);
        issue.type = "security";
        issue.severity = "error";
        issue.filePath = change.filePath;
        issue.description = "Hardcoded password detected";
        issue.suggestion = "Use environment variables or secure vaults";
        issues.append(issue);
    }
    
    return issues;
}

QVector<QualityIssue> CodeQualityAnalyzer::detectPerformanceIssues(const FileChange &change) const
{
    QVector<QualityIssue> issues;
    
    // Detect inefficient loop patterns
    if (change.modifiedContent.contains(QRegularExpression("for.*in.*\\n.*\\n.*\\n.*for.*in"))) {
        QualityIssue issue;
        issue.issueId = QString("perf-nested-loop-%1").arg(change.filePath);
        issue.type = "performance";
        issue.severity = "warning";
        issue.filePath = change.filePath;
        issue.description = "Nested loops detected - potential O(n²) complexity";
        issue.suggestion = "Consider optimizing with better data structures";
        issues.append(issue);
    }
    
    return issues;
}

float CodeQualityAnalyzer::calculateOverallScore(const CodeQualityMetrics &metrics) const
{
    float score = metrics.maintainabilityIndex;

    if (metrics.averageCyclomaticComplexity > m_complexityThreshold) {
        score -= (metrics.averageCyclomaticComplexity - m_complexityThreshold) * 2.0f;
    }

    if (metrics.testCoverage < m_testCoverageThreshold * 100.0f) {
        const float coverageGap = m_testCoverageThreshold * 100.0f - metrics.testCoverage;
        score -= coverageGap * 0.5f;
    }

    if (metrics.duplicationRatio > m_duplicationThreshold) {
        score -= (metrics.duplicationRatio - m_duplicationThreshold) * 120.0f;
    }

    score += qMin(10.0f, metrics.commentRatio * 20.0f);
    score += qMin(10.0f, metrics.performanceScore * 0.05f);
    score -= qMin(20.0f, (100.0f - metrics.securityScore) * 0.5f);

    return std::max(0.0f, std::min(100.0f, score));
}

float CodeQualityAnalyzer::calculateScoreImprovement(const CodeQualityMetrics &before, 
                                                    const CodeQualityMetrics &after) const
{
    float beforeScore = calculateOverallScore(before);
    float afterScore = calculateOverallScore(after);
    return afterScore - beforeScore;
}

void CodeQualityAnalyzer::setComplexityThreshold(float threshold)
{
    m_complexityThreshold = threshold;
}

void CodeQualityAnalyzer::setDuplicationThreshold(float threshold)
{
    m_duplicationThreshold = threshold;
}

void CodeQualityAnalyzer::setTestCoverageThreshold(float threshold)
{
    m_testCoverageThreshold = threshold;
}

int CodeQualityAnalyzer::countLinesOfCode(const QString &code) const
{
    int count = 0;
    for (const auto &line : code.split('\n')) {
        if (!line.trimmed().isEmpty()) {
            ++count;
        }
    }
    return count;
}

int CodeQualityAnalyzer::countCommentedLines(const QString &code) const
{
    int count = 0;
    QStringList lines = code.split('\n');
    
    for (const auto &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("//") || trimmed.startsWith("/*") || trimmed.startsWith("*")) {
            count++;
        }
    }
    
    return count;
}

QVector<QString> CodeQualityAnalyzer::findDuplicatedLines(const FileChange &change) const
{
    QVector<QString> duplicated;
    QStringList lines = change.modifiedContent.split('\n');
    
    // Simple duplication detection
    QMap<QString, int> lineFrequency;
    for (const auto &line : lines) {
        if (line.trimmed().length() > 10) {  // Ignore short lines
            lineFrequency[line]++;
        }
    }
    
    for (auto it = lineFrequency.begin(); it != lineFrequency.end(); ++it) {
        if (it.value() > 2) {  // More than 2 occurrences
            duplicated.append(it.key());
        }
    }
    
    return duplicated;
}
