#include "SkillResourceManager.h"
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QFileInfo>

SkillResourceManager::SkillResourceManager()
    : m_skillsBasePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/skills")
{
}

void SkillResourceManager::setSkillsBasePath(const QString &basePath)
{
    if (basePath.trimmed().isEmpty()) {
        return;
    }

    m_skillsBasePath = QDir::cleanPath(basePath);
}

QString SkillResourceManager::skillsBasePath() const
{
    return m_skillsBasePath;
}

SkillResourceManager::SkillResource SkillResourceManager::loadResource(
    const QString &skillId,
    const QString &resourceName,
    ResourceType type)
{
    SkillResource resource;
    resource.name = resourceName;
    resource.type = type;

    // Determine directory based on resource type
    QString typeDir;
    switch (type) {
    case ResourceType::Script:
        typeDir = "scripts";
        break;
    case ResourceType::Template:
        typeDir = "templates";
        break;
    case ResourceType::Reference:
        typeDir = "references";
        break;
    case ResourceType::Asset:
        typeDir = "assets";
        break;
    }

    // Construct path
    QString resourcePath = QString("%1/%2/%3/%4")
        .arg(m_skillsBasePath, skillId, typeDir, resourceName);
    
    resource.path = resourcePath;

    // Try to load from cache first
    auto cached = getCachedResource(skillId, resourceName, type);
    if (!cached.content.isEmpty()) {
        return cached;
    }

    // Load from filesystem
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to load skill resource:" << resourcePath;
        return resource;
    }

    resource.content = QString::fromUtf8(file.readAll());
    resource.cached = true;
    file.close();

    // Cache the resource
    m_resourceCache[skillId].append(resource);

    return resource;
}

QVector<SkillResourceManager::SkillResource> SkillResourceManager::loadSkillResources(
    const QString &skillId)
{
    QVector<SkillResource> resources;
    
    // Check if already cached
    if (m_resourceCache.contains(skillId)) {
        return m_resourceCache[skillId];
    }

    // Discover all resources for this skill
    QString skillPath = m_skillsBasePath + "/" + skillId;
    QDir skillDir(skillPath);

    // Scan each resource type directory
    QStringList typeDirectories = {"scripts", "templates", "references", "assets"};
    ResourceType typeMap[] = {ResourceType::Script, ResourceType::Template, 
                              ResourceType::Reference, ResourceType::Asset};

    for (int i = 0; i < typeDirectories.count(); ++i) {
        QDir typeDir(skillPath + "/" + typeDirectories[i]);
        if (!typeDir.exists()) continue;

        QStringList files = typeDir.entryList(QDir::Files);
        for (const QString &fileName : files) {
            SkillResource res = loadResource(skillId, fileName, typeMap[i]);
            if (!res.content.isEmpty()) {
                resources.append(res);
            }
        }
    }

    m_resourceCache[skillId] = resources;
    return resources;
}

SkillResourceManager::SkillResource SkillResourceManager::getCachedResource(
    const QString &skillId,
    const QString &resourceName,
    ResourceType type)
{
    SkillResource empty;

    if (!m_resourceCache.contains(skillId)) {
        return empty;
    }

    for (const auto &res : m_resourceCache[skillId]) {
        if (res.name == resourceName && res.type == type) {
            return res;
        }
    }

    return empty;
}

SkillResourceManager::ScriptResult SkillResourceManager::executeScript(
    const ScriptExecution &execution)
{
    ScriptResult result;
    
    // Load the script
    auto scriptRes = loadResource(execution.skillId, execution.scriptName, ResourceType::Script);
    if (scriptRes.content.isEmpty()) {
        result.error = QString("Script not found: %1").arg(execution.scriptName);
        result.success = false;
        return result;
    }

    // Prepare process
    QProcess process;
    
    // Set environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    for (auto it = execution.environment.begin(); it != execution.environment.end(); ++it) {
        env.insert(it.key(), it.value());
    }
    process.setProcessEnvironment(env);
    
    // Set working directory
    if (!execution.workingDirectory.isEmpty()) {
        process.setWorkingDirectory(execution.workingDirectory);
    }

    // Determine script interpreter based on file extension
    QString interpreter;
    const QString scriptName = execution.scriptName;

    if (scriptName.endsWith(".py", Qt::CaseInsensitive)) {
        interpreter = "python3";
    } else if (scriptName.endsWith(".sh", Qt::CaseInsensitive)) {
        interpreter = "/bin/bash";
    } else if (scriptName.endsWith(".js", Qt::CaseInsensitive)
               || scriptName.endsWith(".mjs", Qt::CaseInsensitive)
               || scriptName.endsWith(".cjs", Qt::CaseInsensitive)) {
        interpreter = "node";
    } else if (scriptName.endsWith(".ts", Qt::CaseInsensitive)) {
        if (!QStandardPaths::findExecutable(QStringLiteral("tsx")).isEmpty()) {
            interpreter = "tsx";
        } else if (!QStandardPaths::findExecutable(QStringLiteral("ts-node")).isEmpty()) {
            interpreter = "ts-node";
        } else {
            result.error = "TypeScript script requires 'tsx' or 'ts-node' in PATH";
            result.success = false;
            result.exitCode = -1;
            return result;
        }
    } else {
        interpreter = "/bin/sh";
    }

    // Start process
    QStringList args;
    args << scriptRes.path;
    args << execution.arguments;
    
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    process.start(interpreter, args);

    // Wait for completion with timeout
    if (!process.waitForFinished(execution.timeoutMs)) {
        process.kill();
        process.waitForFinished(1000);
        result.error = "Script execution timeout";
        result.success = false;
        result.exitCode = -1;
    } else {
        result.exitCode = process.exitCode();
        result.success = (result.exitCode == 0);
        
        if (execution.captureOutput) {
            result.stdout = QString::fromUtf8(process.readAllStandardOutput());
            result.stderr = QString::fromUtf8(process.readAllStandardError());
        }
    }

    result.executionTimeMs = QDateTime::currentMSecsSinceEpoch() - startTime;
    return result;
}

void SkillResourceManager::executeScriptAsync(
    const ScriptExecution &execution,
    std::function<void(const ScriptResult &)> callback)
{
    // For now, execute synchronously and call callback
    // In production, would use QThread or similar for true async
    ScriptResult result = executeScript(execution);
    if (callback) {
        callback(result);
    }
}

QString SkillResourceManager::processTemplate(
    const QString &template_content,
    const QMap<QString, QString> &variables)
{
    QString result = template_content;
    
    // Replace variables in format {{variable_name}}
    QRegularExpression regex(R"(\{\{(\w+)\}\})");
    QRegularExpressionMatchIterator iterator = regex.globalMatch(template_content);
    
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString varName = match.captured(1);
        
        if (variables.contains(varName)) {
            result.replace(match.captured(0), variables[varName]);
        }
    }
    
    return result;
}

QString SkillResourceManager::loadAndProcessTemplate(
    const QString &skillId,
    const QString &templateName,
    const QMap<QString, QString> &variables)
{
    auto templateRes = loadResource(skillId, templateName, ResourceType::Template);
    if (templateRes.content.isEmpty()) {
        return QString("Template not found: %1").arg(templateName);
    }
    
    return processTemplate(templateRes.content, variables);
}

void SkillResourceManager::clearCache(const QString &skillId)
{
    m_resourceCache.remove(skillId);
}

void SkillResourceManager::clearAllCaches()
{
    m_resourceCache.clear();
}

QMap<QString, int> SkillResourceManager::getCacheStats() const
{
    QMap<QString, int> stats;
    for (auto it = m_resourceCache.begin(); it != m_resourceCache.end(); ++it) {
        stats[it.key()] = it.value().count();
    }
    return stats;
}
