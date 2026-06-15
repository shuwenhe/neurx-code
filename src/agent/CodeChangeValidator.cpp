#include "CodeChangeValidator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <algorithm>

// ──────────────────────────────────────────────────────────────────────────────
// ValidationViolation Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString ValidationViolation::toJson() const
{
    QJsonObject obj;
    obj["ruleId"] = ruleId;
    obj["severity"] = severity;
    obj["filePath"] = filePath;
    obj["lineNumber"] = lineNumber;
    obj["message"] = message;
    obj["suggestion"] = suggestion;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// ──────────────────────────────────────────────────────────────────────────────
// ValidationResult Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString ValidationResult::toJson() const
{
    QJsonObject obj;
    obj["isValid"] = isValid;
    obj["changeSetId"] = changeSetId;
    obj["errorCount"] = errorCount;
    obj["warningCount"] = warningCount;
    obj["infoCount"] = infoCount;
    obj["validationScore"] = validationScore;
    obj["summary"] = summary;
    
    QJsonArray violationsArray;
    for (const auto &violation : violations) {
        violationsArray.append(QJsonDocument::fromJson(violation.toJson().toUtf8()).object());
    }
    obj["violations"] = violationsArray;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// ──────────────────────────────────────────────────────────────────────────────
// CodeChangeValidator Implementation
// ──────────────────────────────────────────────────────────────────────────────

CodeChangeValidator::CodeChangeValidator()
{
    // Initialize default rules
    ValidationRule namingRule;
    namingRule.ruleId = "naming-convention";
    namingRule.name = "Naming Convention";
    namingRule.description = "Validate file and variable naming conventions";
    namingRule.category = "naming";
    namingRule.priority = 7;
    addRule(namingRule);
    
    ValidationRule sizeRule;
    sizeRule.ruleId = "file-size-limit";
    sizeRule.name = "File Size Limit";
    sizeRule.description = "Validate file size doesn't exceed limits";
    sizeRule.category = "size";
    sizeRule.priority = 8;
    addRule(sizeRule);
    
    ValidationRule complexityRule;
    complexityRule.ruleId = "complexity-threshold";
    complexityRule.name = "Complexity Threshold";
    complexityRule.description = "Validate change complexity is acceptable";
    complexityRule.category = "quality";
    complexityRule.priority = 6;
    addRule(complexityRule);
}

CodeChangeValidator::~CodeChangeValidator()
{
}

void CodeChangeValidator::addRule(const ValidationRule &rule)
{
    m_rules[rule.ruleId] = rule;
}

void CodeChangeValidator::removeRule(const QString &ruleId)
{
    m_rules.remove(ruleId);
}

void CodeChangeValidator::enableRule(const QString &ruleId, bool enable)
{
    auto it = m_rules.find(ruleId);
    if (it != m_rules.end()) {
        it.value().enabled = enable;
    }
}

ValidationResult CodeChangeValidator::validateChange(const FileChange &change) const
{
    ValidationResult result;
    result.isValid = true;
    
    // Check file name
    ValidationViolation violation;
    if (checkNamingConvention(change, violation)) {
        result.violations.append(violation);
        if (violation.severity == "info") {
            result.infoCount++;
        } else if (violation.severity == "warning") {
            result.warningCount++;
        } else {
            result.errorCount++;
            result.isValid = false;
        }
    }
    
    // Check file size
    if (checkFileSize(change, violation)) {
        result.violations.append(violation);
        result.errorCount++;
        result.isValid = false;
    }
    
    // Check complexity
    if (checkComplexity(change, violation)) {
        result.violations.append(violation);
        if (violation.severity == "info") {
            result.infoCount++;
        } else if (violation.severity == "warning") {
            result.warningCount++;
        } else {
            result.errorCount++;
            result.isValid = false;
        }
    }
    
    // Update validation score
    if (result.errorCount > 0) {
        result.validationScore = 0.0f;
    } else if (result.warningCount > 0) {
        result.validationScore = 0.7f;
    } else {
        result.validationScore = 1.0f;
    }
    
    return result;
}

ValidationResult CodeChangeValidator::validateChangeSet(const ChangeSet &changeSet) const
{
    ValidationResult result;
    result.changeSetId = changeSet.changeSetId;
    result.isValid = true;
    
    // Validate file count
    if (changeSet.fileChanges.size() > m_maxFilesPerCommit) {
        ValidationViolation violation;
        violation.ruleId = "file-count-limit";
        violation.severity = "warning";
        violation.message = QString("Too many files in changeset: %1").arg(changeSet.fileChanges.size());
        result.violations.append(violation);
        result.warningCount++;
    }
    
    // Validate commit message
    ValidationViolation commitViolation;
    if (checkCommitMessage(changeSet.commitMessage, commitViolation)) {
        result.violations.append(commitViolation);
        result.errorCount++;
        result.isValid = false;
    }

    // Validate scope and file count
    QString scopeError;
    if (!validateChangeScope(changeSet, scopeError)) {
        ValidationViolation scopeViolation;
        scopeViolation.ruleId = "change-scope";
        scopeViolation.severity = "warning";
        scopeViolation.message = scopeError;
        result.violations.append(scopeViolation);
        result.warningCount++;
    }
    
    // Validate each file change
    for (const auto &change : changeSet.fileChanges) {
        ValidationResult fileResult = validateChange(change);
        result.violations.append(fileResult.violations);
        result.errorCount += fileResult.errorCount;
        result.warningCount += fileResult.warningCount;
        
        if (!fileResult.isValid) {
            result.isValid = false;
        }
    }
    
    // Calculate overall validation score
    if (result.errorCount > 0) {
        result.validationScore = 0.0f;
    } else if (result.warningCount > 0) {
        result.validationScore = 0.6f;
    } else {
        result.validationScore = 1.0f;
    }
    
    result.summary = QString("Validation: %1 errors, %2 warnings, %3 infos")
        .arg(result.errorCount)
        .arg(result.warningCount)
        .arg(result.infoCount);
    
    return result;
}

bool CodeChangeValidator::validateFileName(const QString &filePath, QString &error) const
{
    if (filePath.trimmed().isEmpty()) {
        error = "File path cannot be empty";
        return false;
    }

    // Check for invalid characters
    QRegularExpression invalidChars("[<>:\"|?*\\x00-\\x1F]");
    if (invalidChars.match(filePath).hasMatch()) {
        error = "File path contains invalid characters";
        return false;
    }
    
    // Check path length
    if (filePath.length() > 260) {
        error = "File path exceeds maximum length of 260 characters";
        return false;
    }
    
    return true;
}

bool CodeChangeValidator::validateCommitMessage(const QString &message, QString &error) const
{
    if (message.trimmed().isEmpty()) {
        error = "Commit message cannot be empty";
        return false;
    }

    if (message.length() < m_minCommitMessageLength) {
        error = QString("Commit message too short (minimum %1 characters)").arg(m_minCommitMessageLength);
        return false;
    }
    
    if (!message.contains(QRegularExpression("^[A-Z]"))) {
        error = "Commit message must start with uppercase letter";
        return false;
    }
    
    return true;
}

bool CodeChangeValidator::validateChangeScope(const ChangeSet &changeSet, QString &error) const
{
    // Check if changes are too scattered
    QSet<QString> directories;
    for (const auto &change : changeSet.fileChanges) {
        const int slashPos = change.filePath.lastIndexOf('/');
        QString dir = slashPos >= 0 ? change.filePath.left(slashPos) : QStringLiteral(".");
        directories.insert(dir);
    }
    
    if (directories.size() > 10) {
        error = "Changes span too many directories (more than 10)";
        return false;
    }
    
    return true;
}

bool CodeChangeValidator::validateFileSizeLimit(const FileChange &change, int maxSizeKb, QString &error) const
{
    int sizeKb = change.fileSize / 1024;
    if (sizeKb > maxSizeKb) {
        error = QString("File size exceeds limit: %1KB > %2KB").arg(sizeKb).arg(maxSizeKb);
        return false;
    }
    
    return true;
}

QVector<ValidationRule> CodeChangeValidator::getAllRules() const
{
    return m_rules.values().toVector();
}

QVector<ValidationRule> CodeChangeValidator::getRulesByCategory(const QString &category) const
{
    QVector<ValidationRule> result;
    for (const auto &rule : m_rules) {
        if (rule.category == category) {
            result.append(rule);
        }
    }
    return result;
}

void CodeChangeValidator::setMaxFileSizeKb(int sizeKb)
{
    m_maxFileSizeKb = sizeKb;
}

void CodeChangeValidator::setMaxFilesPerCommit(int count)
{
    m_maxFilesPerCommit = count;
}

void CodeChangeValidator::setMinCommitMessageLength(int length)
{
    m_minCommitMessageLength = length;
}

void CodeChangeValidator::setMaxLinesPerFile(int lines)
{
    m_maxLinesPerFile = lines;
}

bool CodeChangeValidator::checkNamingConvention(const FileChange &change, ValidationViolation &violation) const
{
    auto it = m_rules.find("naming-convention");
    if (it == m_rules.end() || !it.value().enabled) {
        return false;
    }
    
    // Check file extension
    const int dotPos = change.filePath.lastIndexOf('.');
    const QString extension = dotPos >= 0 ? change.filePath.mid(dotPos + 1) : QString();
    if (extension.isEmpty()) {
        violation.ruleId = "naming-convention";
        violation.severity = "info";
        violation.filePath = change.filePath;
        violation.message = "File has no extension";
        return true;
    }
    
    // Validate against whitelist
    QStringList validExtensions = {"cpp", "h", "ts", "js", "py", "java", "cs", "go", "rs"};
    if (!validExtensions.contains(extension.toLower())) {
        violation.ruleId = "naming-convention";
        violation.severity = "info";
        violation.filePath = change.filePath;
        violation.message = QString("Uncommon file extension: .%1").arg(extension);
        return true;
    }
    
    return false;
}

bool CodeChangeValidator::checkFileSize(const FileChange &change, ValidationViolation &violation) const
{
    auto it = m_rules.find("file-size-limit");
    if (it == m_rules.end() || !it.value().enabled) {
        return false;
    }
    
    int sizeKb = change.fileSize / 1024;
    if (sizeKb > m_maxFileSizeKb) {
        violation.ruleId = "file-size-limit";
        violation.severity = "error";
        violation.filePath = change.filePath;
        violation.message = QString("File size exceeds limit: %1KB > %2KB")
            .arg(sizeKb).arg(m_maxFileSizeKb);
        return true;
    }
    
    return false;
}

bool CodeChangeValidator::checkCommitMessage(const QString &message, ValidationViolation &violation) const
{
    if (message.length() < m_minCommitMessageLength) {
        violation.ruleId = "commit-message";
        violation.severity = "error";
        violation.message = QString("Commit message too short (minimum %1 characters)")
            .arg(m_minCommitMessageLength);
        return true;
    }
    
    return false;
}

bool CodeChangeValidator::checkComplexity(const FileChange &change, ValidationViolation &violation) const
{
    auto it = m_rules.find("complexity-threshold");
    if (it == m_rules.end() || !it.value().enabled) {
        return false;
    }
    
    if (change.changeComplexity > 0.8f) {
        violation.ruleId = "complexity-threshold";
        violation.severity = "warning";
        violation.filePath = change.filePath;
        violation.message = QString("Change complexity is high: %1%")
            .arg(static_cast<int>(change.changeComplexity * 100));
        return true;
    }

    const int lineCount = change.modifiedContent.split('\n').size();
    if (lineCount > m_maxLinesPerFile) {
        violation.ruleId = "line-count-limit";
        violation.severity = "warning";
        violation.filePath = change.filePath;
        violation.message = QString("File exceeds line limit: %1 > %2")
            .arg(lineCount)
            .arg(m_maxLinesPerFile);
        return true;
    }
    
    return false;
}
