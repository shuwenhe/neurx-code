#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include "CodeChangeTracker.h"

/**
 * @class CodeChangeValidator
 * @brief Validates code changes against policies and best practices
 * 
 * Features:
 * - File naming conventions
 * - Commit message validation
 * - Change scope analysis
 * - Dependency impact analysis
 * - Policy enforcement
 */

// ──────────────────────────────────────────────────────────────────────────────
// Validation Rule Types
// ──────────────────────────────────────────────────────────────────────────────

struct ValidationRule {
    QString ruleId;
    QString name;
    QString description;
    bool enabled{true};
    int priority{5};                    // 1-10, higher = more important
    QString category;                   // "naming", "size", "security", "quality"
};

struct ValidationViolation {
    QString ruleId;
    QString severity;                   // "info", "warning", "error"
    QString filePath;
    int lineNumber{0};
    QString message;
    QString suggestion;
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Validation Result
// ──────────────────────────────────────────────────────────────────────────────

struct ValidationResult {
    bool isValid{true};
    QVector<ValidationViolation> violations;
    
    int errorCount{0};
    int warningCount{0};
    int infoCount{0};
    
    QString changeSetId;
    float validationScore{1.0f};        // 0-1 validation score
    
    QString summary;
    
    QString toJson() const;
};

// ──────────────────────────────────────────────────────────────────────────────
// Code Change Validator
// ──────────────────────────────────────────────────────────────────────────────

class CodeChangeValidator {
public:
    explicit CodeChangeValidator();
    ~CodeChangeValidator();

    // Rule management
    void addRule(const ValidationRule &rule);
    void removeRule(const QString &ruleId);
    void enableRule(const QString &ruleId, bool enable);
    
    // Validation
    ValidationResult validateChange(const FileChange &change) const;
    ValidationResult validateChangeSet(const ChangeSet &changeSet) const;
    
    // Specific validations
    bool validateFileName(const QString &filePath, QString &error) const;
    bool validateCommitMessage(const QString &message, QString &error) const;
    bool validateChangeScope(const ChangeSet &changeSet, QString &error) const;
    bool validateFileSizeLimit(const FileChange &change, int maxSizeKb, QString &error) const;
    
    // Rules query
    QVector<ValidationRule> getAllRules() const;
    QVector<ValidationRule> getRulesByCategory(const QString &category) const;
    
    // Policy configuration
    void setMaxFileSizeKb(int sizeKb);
    void setMaxFilesPerCommit(int count);
    void setMinCommitMessageLength(int length);
    void setMaxLinesPerFile(int lines);
    
private:
    // Validation checks
    bool checkNamingConvention(const FileChange &change, ValidationViolation &violation) const;
    bool checkFileSize(const FileChange &change, ValidationViolation &violation) const;
    bool checkCommitMessage(const QString &message, ValidationViolation &violation) const;
    bool checkComplexity(const FileChange &change, ValidationViolation &violation) const;
    
    // Member variables
    QMap<QString, ValidationRule> m_rules;
    
    int m_maxFileSizeKb{10 * 1024};     // 10 MB default
    int m_maxFilesPerCommit{100};
    int m_minCommitMessageLength{10};
    int m_maxLinesPerFile{5000};
};
