#include "tools/AgentFileWriterTool.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QRegularExpression>

AgentFileWriterTool::AgentFileWriterTool(const QString &workspaceRoot, QObject *parent)
    : BaseTool(parent), m_workspaceRoot(workspaceRoot) {}

QJsonObject AgentFileWriterTool::parametersSchema() const
{
    QJsonObject schema;
    schema["type"] = "object";
    
    QJsonObject properties;
    QJsonObject operation;
    operation["type"] = "string";
    QJsonArray operationEnum;
    operationEnum << "write_single" << "write_batch" << "update_file" << "write_template" << "create_structure";
    operation["enum"] = operationEnum;
    operation["description"] = "Type of file operation";
    properties["operation"] = operation;
    
    properties["path"] = QJsonObject{{"type", "string"}, {"description", "File path relative to workspace, or an absolute path inside the workspace"}};
    properties["content"] = QJsonObject{{"type", "string"}, {"description", "File content"}};
    
    QJsonObject mode;
    mode["type"] = "string";
    QJsonArray modeEnum;
    modeEnum << "overwrite" << "append" << "prepend" << "insert";
    mode["enum"] = modeEnum;
    properties["mode"] = mode;
    
    properties["line"] = QJsonObject{{"type", "integer"}, {"description", "Line number for insert"}};
    properties["files"] = QJsonObject{{"type", "array"}, {"description", "Array of file objects"}};
    properties["template_name"] = QJsonObject{{"type", "string"}};
    properties["variables"] = QJsonObject{{"type", "object"}};
    properties["structure"] = QJsonObject{{"type", "object"}};
    properties["create_dirs"] = QJsonObject{{"type", "boolean"}, {"description", "Create parent directories"}};
    properties["backup"] = QJsonObject{{"type", "boolean"}, {"description", "Create backup"}};
    properties["atomic"] = QJsonObject{{"type", "boolean"}, {"description", "All-or-nothing batch"}};
    properties["validate"] = QJsonObject{{"type", "boolean"}, {"description", "Validate content"}};
    
    schema["properties"] = properties;
    
    QJsonArray required;
    required << "operation";
    schema["required"] = required;
    
    return schema;
}

ToolResult AgentFileWriterTool::execute(const QString &callId, const QJsonObject &args)
{
    // Make 'operation' optional - default to write_single if not provided
    QString operation = args["operation"].toString();
    if (operation.isEmpty()) {
        // If path and content are provided without operation, assume write_single
        if (args.contains("path") && args.contains("content")) {
            qDebug() << "[AgentFileWriterTool] No operation specified, defaulting to write_single";
            operation = "write_single";
        }
    }
    
    qDebug() << "[AgentFileWriterTool] execute() called with operation:" << operation 
             << "path:" << args["path"].toString();
    
    if (operation == "write_single") return opWriteSingle(callId, args);
    else if (operation == "write_batch") return opWriteBatch(callId, args);
    else if (operation == "update_file") return opUpdateFile(callId, args);
    else if (operation == "write_template") return opWriteTemplate(callId, args);
    else if (operation == "create_structure") return opCreateStructure(callId, args);
    
    qDebug() << "[AgentFileWriterTool] Unknown operation:" << operation;
    return {callId, name(), true, "Unknown operation: " + operation};
}

QString AgentFileWriterTool::summary(const QJsonObject &args) const
{
    return args["operation"].toString() + ": " + args["path"].toString();
}

ToolResult AgentFileWriterTool::opWriteSingle(const QString &callId, const QJsonObject &args)
{
    const QString path = args["path"].toString();
    const QString content = args["content"].toString();
    const bool createDirs = args.value("create_dirs").toBool(true);
    const bool backup = args.value("backup").toBool(true);
    const bool validateContent = args.value("validate").toBool(true);

    qDebug() << "[AgentFileWriterTool::opWriteSingle] Writing to:" << path 
             << "size:" << content.length() << "bytes";

    const QString safeFilePath = safePath(path);
    if (safeFilePath.isEmpty()) {
        qDebug() << "[AgentFileWriterTool::opWriteSingle] Path traversal detected for:" << path;
        return {callId, name(), true, "Path traversal detected"};
    }

    if (validateContent) {
        QString validationError;
        if (!this->validateContent(content, validationError))
            return {callId, name(), true, "Validation failed: " + validationError};
    }

    if (createDirs) {
        QString dirError;
        if (!ensureDirectories(safeFilePath, dirError))
            return {callId, name(), true, "Directory creation failed"};
    }

    QString backupPath;
    if (backup && QFileInfo::exists(safeFilePath)) {
        backupPath = createBackup(safeFilePath);
    }

    QString writeError;
    if (!writeFileAtomically(safeFilePath, content, writeError)) {
        qDebug() << "[AgentFileWriterTool::opWriteSingle] Write failed:" << writeError;
        if (!backupPath.isEmpty() && QFileInfo::exists(backupPath)) {
            QFile::remove(safeFilePath);
            QFile::rename(backupPath, safeFilePath);
            qDebug() << "[AgentFileWriterTool::opWriteSingle] Restored from backup";
        }
        return {callId, name(), true, "Write failed: " + writeError};
    }

    qDebug() << "[AgentFileWriterTool::opWriteSingle] File written successfully:" << safeFilePath 
             << "size:" << content.length() << "bytes";

    QJsonObject result;
    result["path"] = path;
    result["size"] = static_cast<int>(content.length());
    result["status"] = "success";
    return {callId, name(), false, QJsonDocument(result).toJson()};
}

ToolResult AgentFileWriterTool::opWriteBatch(const QString &callId, const QJsonObject &args)
{
    const QJsonArray filesArray = args["files"].toArray();
    const bool atomic = args.value("atomic").toBool(true);

    if (filesArray.isEmpty()) return {callId, name(), true, "No files in batch"};
    if (filesArray.size() > MAX_BATCH_FILES) return {callId, name(), true, "Batch exceeds limit"};

    QMap<QString, QString> backups;
    QStringList written;

    for (int i = 0; i < filesArray.size(); ++i) {
        const QJsonObject fileObj = filesArray[i].toObject();
        const QString filePath = fileObj["path"].toString();
        const QString content = fileObj["content"].toString();
        const QString safeFilePath = safePath(filePath);
        
        if (safeFilePath.isEmpty()) {
            if (atomic) {
                for (const QString &p : written) {
                    QFile::remove(p);
                    if (!backups[p].isEmpty()) QFile::rename(backups[p], p);
                }
            }
            return {callId, name(), true, "Invalid path"};
        }

        if (QFileInfo::exists(safeFilePath)) {
            backups[safeFilePath] = createBackup(safeFilePath);
        }

        QString writeError;
        if (!writeFileAtomically(safeFilePath, content, writeError)) {
            if (atomic) {
                for (const QString &p : written) {
                    QFile::remove(p);
                    if (!backups[p].isEmpty()) QFile::rename(backups[p], p);
                }
            }
            return {callId, name(), true, "Write failed"};
        }
        written.append(safeFilePath);
    }

    QJsonObject result;
    result["count"] = filesArray.size();
    result["status"] = "success";
    return {callId, name(), false, QJsonDocument(result).toJson()};
}

ToolResult AgentFileWriterTool::opUpdateFile(const QString &callId, const QJsonObject &args)
{
    const QString path = args["path"].toString();
    const QString content = args["content"].toString();
    const QString modeStr = args.value("mode").toString("append");
    const int lineNum = args["line"].toInt(1);

    const QString safeFilePath = safePath(path);
    if (safeFilePath.isEmpty()) return {callId, name(), true, "Invalid path"};
    if (!QFileInfo::exists(safeFilePath)) return {callId, name(), true, "File not found"};

    QFile file(safeFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {callId, name(), true, "Cannot read file"};

    QString existingContent = QString::fromUtf8(file.readAll());
    file.close();

    qint64 oldSize = existingContent.length();
    QString newContent;

    if (modeStr == "append") {
        newContent = existingContent + content;
    } else if (modeStr == "prepend") {
        newContent = content + existingContent;
    } else if (modeStr == "insert") {
        QStringList lines = existingContent.split('\n');
        if (lineNum < 1 || lineNum > lines.size() + 1)
            return {callId, name(), true, "Invalid line number"};
        lines.insert(lineNum - 1, content);
        newContent = lines.join('\n');
    } else if (modeStr == "overwrite") {
        newContent = content;
    } else {
        return {callId, name(), true, "Unknown mode"};
    }

    QString backupPath = createBackup(safeFilePath);
    QString writeError;
    if (!writeFileAtomically(safeFilePath, newContent, writeError)) {
        if (!backupPath.isEmpty() && QFileInfo::exists(backupPath)) {
            QFile::remove(safeFilePath);
            QFile::rename(backupPath, safeFilePath);
        }
        return {callId, name(), true, "Write failed"};
    }

    QJsonObject result;
    result["path"] = path;
    result["old_size"] = static_cast<int>(oldSize);
    result["new_size"] = static_cast<int>(newContent.length());
    return {callId, name(), false, QJsonDocument(result).toJson()};
}

ToolResult AgentFileWriterTool::opWriteTemplate(const QString &callId, const QJsonObject &args)
{
    const QString path = args["path"].toString();
    const QString templateName = args["template_name"].toString();
    const QJsonObject variables = args["variables"].toObject();
    QString templateContent = "// Template: " + templateName + "\n";
    templateContent = processTemplate(templateContent, variables);
    return opWriteSingle(callId, QJsonObject{{QStringLiteral("path"), path}, {QStringLiteral("content"), templateContent}});
}

ToolResult AgentFileWriterTool::opCreateStructure(const QString &callId, const QJsonObject &args)
{
    const QJsonObject structure = args["structure"].toObject();
    int dirsCreated = 0, filesCreated = 0;
    for (auto it = structure.begin(); it != structure.end(); ++it) {
        if (it.value().isObject()) {
            QString dirPath = safePath(it.key());
            if (!dirPath.isEmpty()) {
                QString error;
                if (ensureDirectories(dirPath, error)) dirsCreated++;
            }
        } else if (it.value().isString()) {
            QString filePath = safePath(it.key());
            if (!filePath.isEmpty()) {
                QString error;
                if (writeFileAtomically(filePath, it.value().toString(), error)) filesCreated++;
            }
        }
    }
    QJsonObject result;
    result["directories"] = dirsCreated;
    result["files"] = filesCreated;
    return {callId, name(), false, QJsonDocument(result).toJson()};
}

QString AgentFileWriterTool::safePath(const QString &relativePath) const
{
    const QString rawPath = relativePath.trimmed();
    if (rawPath.isEmpty())
        return {};

    const QString workspaceRoot = QDir(m_workspaceRoot).absolutePath();
    const QFileInfo pathInfo(rawPath);
    const QString absPath = pathInfo.isAbsolute()
        ? QDir::cleanPath(pathInfo.absoluteFilePath())
        : QDir(workspaceRoot).absoluteFilePath(rawPath);
    const QString normalizedAbsPath = QDir::cleanPath(absPath);

    if (normalizedAbsPath == workspaceRoot)
        return {};

    const QString workspacePrefix = workspaceRoot.endsWith('/')
        ? workspaceRoot
        : workspaceRoot + '/';
    if (!normalizedAbsPath.startsWith(workspacePrefix))
        return {};

    return normalizedAbsPath;
}

bool AgentFileWriterTool::writeFileAtomically(const QString &filePath, const QString &content, QString &errorMsg)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errorMsg = file.errorString();
        return false;
    }
    QTextStream out(&file);
    out << content;
    out.flush();
    if (!file.commit()) {
        errorMsg = file.errorString();
        return false;
    }
    return true;
}

bool AgentFileWriterTool::ensureDirectories(const QString &filePath, QString &errorMsg)
{
    QFileInfo info(filePath);
    QDir dir;
    if (!dir.mkpath(info.absolutePath())) {
        errorMsg = "Cannot create directories";
        return false;
    }
    return true;
}

QString AgentFileWriterTool::createBackup(const QString &filePath)
{
    QFileInfo info(filePath);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    const QString backupPath = filePath + ".backup." + timestamp;
    if (QFile::copy(filePath, backupPath)) return backupPath;
    return {};
}

bool AgentFileWriterTool::validateContent(const QString &content, QString &errorMsg) const
{
    if (content.length() > MAX_FILE_SIZE) {
        errorMsg = "Content too large";
        return false;
    }
    if (content.contains('\0')) {
        errorMsg = "Content contains null bytes";
        return false;
    }
    return true;
}

QString AgentFileWriterTool::processTemplate(const QString &templateContent, const QJsonObject &variables) const
{
    QString result = templateContent;
    QRegularExpression re(R"(\{\{([A-Z_]+)\}\})");
    QRegularExpressionMatchIterator it = re.globalMatch(templateContent);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString varName = match.captured(1);
        const QString varValue = variables.value(varName).toString();
        if (!varValue.isEmpty())
            result.replace(match.captured(0), varValue);
    }
    return result;
}
