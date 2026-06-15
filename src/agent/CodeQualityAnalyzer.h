#pragma once

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include "CodeChangeTracker.h"

/**
 * @class CodeQualityAnalyzer
 * @brief Analyzes code quality metrics and identifies issues
 * 
 * Features:
 * - Code smell detection
 * - Cyclomatic complexity analysis
 * - Code duplication detection
 * - Test coverage tracking
 * - Performance analysis
 * - Security analysis
 */

// ──────────────────────────────────────────────────────────────────────────────
// Code Quality Metrics
// ──────────────────────────────────────────────────────────────────────────────

struct CodeQualityMetrics {
    // Complexity metrics
    float averageCyclomaticComplexity{0.0f};    // Lower is better
    float maxCyclomaticComplexity{0.0f};
    
    // Size metrics
    int linesOfCode{0};
    int commentedLines{0};
    float commentRatio{0.0f};               // Comments / LOC
    
    // Duplication metrics
    float duplicationRatio{0.0f};           // Duplicated lines / total lines
    int duplicatedBlocks{0};
    
    // Maintainability metrics
    float maintainabilityIndex{0.0f};       // 0-100, higher is better
    
    // Test coverage
    float testCoverage{0.0f};               // 0-100%
    int uncoveredLines{0};
    
    // Performance
    float performanceScore{0.0f};           // 0-100
    QString performanceIssues;
    
    // Security
    float securityScore{0.0f};              // 0-100
    QVector<QString> securityVulnerabilities;
};

struct QualityIssue {
    QString issueId;
    QString type;                          // "smell", "complexity", "duplication", "security"
    QString severity;                       // "info", "warning", "error"
    QString filePath;
    int lineNumber{0};
    QString description;
    QString suggestion;
};

// ──────────────────────────────────────────────────────────────────────────────
// Code Quality Report
// ──────────────────────────────────────────────────────────────────────────────

struct CodeQualityReport {
    QString reportId;
    QString changeSetId;
    
    CodeQualityMetrics beforeMetrics;
    CodeQualityMetrics afterMetrics;
    
    QVector<QualityIssue> issues;
    
    int criticalIssues{0};
    int warnings{0};
    int suggestions{0};
    
    float overallScore{0.0f};               // 0-100
    float scoreImprovement{0.0f};           // Change in score
    
    QString summary;
    QString generatedAt;
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Code Quality Analyzer
// ──────────────────────────────────────────────────────────────────────────────

class CodeQualityAnalyzer {
public:
    explicit CodeQualityAnalyzer();
    ~CodeQualityAnalyzer();

    // Analysis
    CodeQualityMetrics analyzeFile(const FileChange &change) const;
    CodeQualityReport analyzeChangeSet(const ChangeSet &changeSet) const;
    
    // Specific analyses
    float calculateCyclomaticComplexity(const QString &code) const;
    float calculateMaintainabilityIndex(const FileChange &change) const;
    QVector<QualityIssue> detectCodeSmells(const FileChange &change) const;
    QVector<QualityIssue> detectSecurityIssues(const FileChange &change) const;
    QVector<QualityIssue> detectPerformanceIssues(const FileChange &change) const;
    
    // Scoring
    float calculateOverallScore(const CodeQualityMetrics &metrics) const;
    float calculateScoreImprovement(const CodeQualityMetrics &before, 
                                   const CodeQualityMetrics &after) const;
    
    // Configuration
    void setComplexityThreshold(float threshold);
    void setDuplicationThreshold(float threshold);
    void setTestCoverageThreshold(float threshold);
    
private:
    // Helper methods
    int countLinesOfCode(const QString &code) const;
    int countCommentedLines(const QString &code) const;
    QVector<QString> findDuplicatedLines(const FileChange &change) const;
    
    // Configuration
    float m_complexityThreshold{10.0f};
    float m_duplicationThreshold{0.10f};   // 10% duplication
    float m_testCoverageThreshold{0.80f};  // 80% coverage
};
