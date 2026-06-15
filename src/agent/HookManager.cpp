#include "HookManager.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QTextStream>
#include <QDir>
#include <QFileSystemWatcher>
#include <QFileInfo>
#include <QRegularExpression>

// ── 构造和析构 ──────────────────────────────────────────────────────────────

HookManager::HookManager(QObject *parent)
    : QObject(parent)
{
    qInfo() << "[HookManager] Initialized";
    // 初始化内置安全规则
    setupDefaultSecurityRules();
}

HookManager::~HookManager()
{
    qInfo() << "[HookManager] Destroyed";
}

// ── Hook 注册和管理 ─────────────────────────────────────────────────────────

void HookManager::registerHook(const HookConfig& config)
{
    if (config.name.isEmpty()) {
        qWarning() << "[HookManager] Cannot register hook with empty name";
        return;
    }

    m_hooks[config.name] = config;
    m_hooksByType[config.type].append(config.name);

    qInfo() << "[HookManager] Registered hook:" << config.name 
            << "type:" << hookTypeToString(config.type)
            << "mode:" << (config.mode == HookMode::PromptBased ? "prompt" : "command");
}

void HookManager::unregisterHook(const QString& name)
{
    if (!m_hooks.contains(name)) {
        qWarning() << "[HookManager] Hook not found:" << name;
        return;
    }

    HookType type = m_hooks[name].type;
    m_hooksByType[type].removeAll(name);
    m_hooks.remove(name);

    qInfo() << "[HookManager] Unregistered hook:" << name;
}

void HookManager::setHookEnabled(const QString& name, bool enabled)
{
    if (!m_hooks.contains(name)) {
        qWarning() << "[HookManager] Hook not found:" << name;
        return;
    }

    m_hooks[name].enabled = enabled;
    qInfo() << "[HookManager] Hook" << name << (enabled ? "enabled" : "disabled");
}

QList<HookManager::HookConfig> HookManager::allHooks() const
{
    QList<HookConfig> values = m_hooks.values();
    return values;
}

QList<HookManager::HookConfig> HookManager::hooksForType(HookType type) const
{
    QList<HookConfig> result;
    if (m_hooksByType.contains(type)) {
        for (const QString& name : m_hooksByType[type]) {
            if (m_hooks.contains(name)) {
                result.append(m_hooks[name]);
            }
        }
    }
    return result;
}

// ── Hook 执行 ───────────────────────────────────────────────────────────────

QList<HookManager::HookResult> HookManager::executeHooks(HookType type, const QJsonObject& context)
{
    QList<HookResult> results;
    
    QList<HookConfig> hooks = hooksForType(type);
    qInfo() << "[HookManager] Executing" << hooks.size() << "hooks for type:" << hookTypeToString(type);

    for (const HookConfig& hook : hooks) {
        if (!hook.enabled) {
            qDebug() << "[HookManager] Skipping disabled hook:" << hook.name;
            continue;
        }

        HookResult result = executeHook(hook, context);
        results.append(result);

        emit hookExecuted(type, hook.name, result);

        // 如果任何 hook 阻止操作，记录日志
        if (result.blockOperation) {
            qWarning() << "[HookManager] Hook" << hook.name << "blocked operation";
        }
    }

    return results;
}

HookManager::HookResult HookManager::executeHook(const HookConfig& hook, const QJsonObject& context)
{
    qDebug() << "[HookManager] Executing hook:" << hook.name;

    try {
        if (hook.mode == HookMode::PromptBased) {
            return executePromptHook(hook, context);
        } else {
            return executeCommandHook(hook, context);
        }
    } catch (const std::exception& e) {
        qCritical() << "[HookManager] Hook execution failed:" << hook.name << e.what();
        
        HookResult errorResult;
        errorResult.systemMessage = QString("Hook '%1' failed: %2").arg(hook.name, e.what());
        errorResult.blockOperation = false;  // 错误时不阻止操作
        
        emit hookError(hook.type, hook.name, e.what());
        
        return errorResult;
    }
}

// ── Prompt-based Hook 执行 ──────────────────────────────────────────────────

HookManager::HookResult HookManager::executePromptHook(const HookConfig& hook, const QJsonObject& context)
{
    qDebug() << "[HookManager] Executing prompt-based hook:" << hook.name;

    // TODO: 集成 LLM 进行决策
    // 这里需要：
    // 1. 将 hook.hookPrompt 和 context 发送给 LLM
    // 2. 解析 LLM 返回的 JSON 格式响应
    // 3. 提取 systemMessage, userMessage, blockOperation

    HookResult result;
    result.systemMessage = QString("⚠️ Prompt-based hook '%1' executed (LLM integration pending)").arg(hook.name);
    result.userMessage = "Hook evaluation requires LLM integration";
    result.blockOperation = false;

    qInfo() << "[HookManager] Prompt hook" << hook.name << "completed (stub)";
    
    return result;
}

// ── Command-based Hook 执行 ─────────────────────────────────────────────────

HookManager::HookResult HookManager::executeCommandHook(const HookConfig& hook, const QJsonObject& context)
{
    qDebug() << "[HookManager] Executing command-based hook:" << hook.name;

    HookResult result;

    // 准备命令
    QString command = expandVariables(hook.command, context);
    QStringList args = hook.args;
    for (QString& arg : args) {
        arg = expandVariables(arg, context);
    }

    // 执行命令
    QProcess process;
    if (!hook.workingDir.isEmpty()) {
        process.setWorkingDirectory(expandVariables(hook.workingDir, context));
    }

    // 传递 context 作为 stdin（JSON 格式）
    process.start(command, args);
    
    if (!process.waitForStarted(1000)) {
        QString error = QString("Failed to start command: %1").arg(process.errorString());
        qCritical() << "[HookManager]" << error;
        result.systemMessage = error;
        result.blockOperation = false;
        return result;
    }

    // 写入 context
    QJsonDocument doc(context);
    process.write(doc.toJson());
    process.closeWriteChannel();

    // 等待完成
    if (!process.waitForFinished(hook.timeout)) {
        qWarning() << "[HookManager] Hook timeout:" << hook.name;
        process.kill();
        result.systemMessage = QString("Hook '%1' timed out").arg(hook.name);
        result.blockOperation = false;
        return result;
    }

    // 读取输出
    result.exitCode = process.exitCode();
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QString errorOutput = QString::fromUtf8(process.readAllStandardError());

    if (!errorOutput.isEmpty()) {
        qWarning() << "[HookManager] Hook stderr:" << errorOutput;
    }

    // 解析输出（期望 JSON 格式）
    QJsonObject outputJson = parseHookOutput(output);
    
    result.systemMessage = outputJson.value("systemMessage").toString();
    result.userMessage = outputJson.value("userMessage").toString();
    result.blockOperation = outputJson.value("blockOperation").toBool(false);
    result.metadata = outputJson.value("metadata").toObject();

    qInfo() << "[HookManager] Command hook" << hook.name 
            << "completed with exit code:" << result.exitCode
            << "block:" << result.blockOperation;

    return result;
}

// ── 便捷方法 ────────────────────────────────────────────────────────────────

bool HookManager::shouldAllowToolUse(const QString& toolName, const QJsonObject& toolInput)
{
    QJsonObject context;
    context["tool_name"] = toolName;
    context["tool_input"] = toolInput;

    QList<HookResult> results = executeHooks(HookType::PreToolUse, context);

    // 如果任何 hook 阻止，返回 false
    for (const HookResult& result : results) {
        if (result.blockOperation) {
            qWarning() << "[HookManager] Tool use blocked for:" << toolName;
            return false;
        }
    }

    return true;
}

QString HookManager::getSessionStartPrompt()
{
    QJsonObject context;
    context["event"] = "session_start";

    QList<HookResult> results = executeHooks(HookType::SessionStart, context);

    QStringList prompts;
    for (const HookResult& result : results) {
        if (!result.systemMessage.isEmpty()) {
            prompts.append(result.systemMessage);
        }
    }

    return prompts.join("\n\n");
}

bool HookManager::shouldContinueSession(const QJsonObject& context)
{
    QList<HookResult> results = executeHooks(HookType::Stop, context);

    // 如果任何 hook 阻止退出，返回 true（继续会话）
    for (const HookResult& result : results) {
        if (result.blockOperation) {
            qInfo() << "[HookManager] Session exit blocked, continuing...";
            return true;
        }
    }

    return false;
}

// ── 辅助方法 ────────────��───────────────────────────────────────────────────

QString HookManager::expandVariables(const QString& text, const QJsonObject& context)
{
    QString result = text;

    // 展开环境变量
    result.replace("${HOME}", QDir::homePath());
    result.replace("${PWD}", QDir::currentPath());

    // 展开 context 中的变量
    for (const QString& key : context.keys()) {
        QString placeholder = QString("${%1}").arg(key);
        QJsonValue val = context.value(key);
        if (val.isObject()) {
            // 展开嵌套对象（如 tool_input.path）
            QJsonObject obj = val.toObject();
            for (const QString& subKey : obj.keys()) {
                QString subPlaceholder = QString("${%1.%2}").arg(key, subKey);
                result.replace(subPlaceholder, obj.value(subKey).toVariant().toString());
            }
        }
        result.replace(placeholder, val.toVariant().toString());
    }

    return result;
}

QJsonObject HookManager::parseHookOutput(const QString& output)
{
    // 尝试解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    
    if (!doc.isNull() && doc.isObject()) {
        return doc.object();
    }

    // 如果不�� JSON，构造一个简单的对象
    QJsonObject result;
    result["systemMessage"] = output.trimmed();
    result["blockOperation"] = false;
    
    return result;
}

// ── 辅助函数实现 ────────────────────────────────────────────────────────────

QString hookTypeToString(HookManager::HookType type)
{
    switch (type) {
        case HookManager::HookType::PreToolUse: return "PreToolUse";
        case HookManager::HookType::PostToolUse: return "PostToolUse";
        case HookManager::HookType::SessionStart: return "SessionStart";
        case HookManager::HookType::SessionEnd: return "SessionEnd";
        case HookManager::HookType::Stop: return "Stop";
        case HookManager::HookType::SubagentStop: return "SubagentStop";
        case HookManager::HookType::UserPromptSubmit: return "UserPromptSubmit";
        case HookManager::HookType::PreCompact: return "PreCompact";
        case HookManager::HookType::Notification: return "Notification";
        default: return "Unknown";
    }
}

HookManager::HookType hookTypeFromString(const QString& str)
{
    if (str == "PreToolUse") return HookManager::HookType::PreToolUse;
    if (str == "PostToolUse") return HookManager::HookType::PostToolUse;
    if (str == "SessionStart") return HookManager::HookType::SessionStart;
    if (str == "SessionEnd") return HookManager::HookType::SessionEnd;
    if (str == "Stop") return HookManager::HookType::Stop;
    if (str == "SubagentStop") return HookManager::HookType::SubagentStop;
    if (str == "UserPromptSubmit") return HookManager::HookType::UserPromptSubmit;
    if (str == "PreCompact") return HookManager::HookType::PreCompact;
    if (str == "Notification") return HookManager::HookType::Notification;
    
    qWarning() << "[HookManager] Unknown hook type:" << str;
    return HookManager::HookType::PreToolUse;
}

void HookManager::loadHooksFromDirectory(const QString& directoryPath)
{
    QDir dir(directoryPath);
    if (!dir.exists()) {
        qWarning() << "[HookManager] Directory does not exist:" << directoryPath;
        return;
    }

    qInfo() << "[HookManager] Loading hooks from:" << directoryPath;

    QStringList filters;
    filters << "*.hook.md" << "*.hook.json";

    for (const QFileInfo& fileInfo : dir.entryInfoList(filters, QDir::Files)) {
        HookConfig config = loadHookFromFile(fileInfo.absoluteFilePath());
        if (!config.name.isEmpty()) {
            registerHook(config);
        }
    }
}

void HookManager::watchDirectory(const QString& directoryPath)
{
    static QFileSystemWatcher* watcher = nullptr;
    if (!watcher) {
        watcher = new QFileSystemWatcher(this);
        connect(watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString& path) {
            qInfo() << "[HookManager] Directory changed, reloading hooks:" << path;
            loadHooksFromDirectory(path);
        });
        connect(watcher, &QFileSystemWatcher::fileChanged, this, [this, directoryPath](const QString& path) {
            qInfo() << "[HookManager] Hook file changed, reloading directory:" << directoryPath;
            loadHooksFromDirectory(directoryPath);
        });
    }

    if (QDir(directoryPath).exists()) {
        watcher->addPath(directoryPath);
        loadHooksFromDirectory(directoryPath);
    }
}

HookManager::HookConfig loadHookFromFile(const QString& filePath)
{
    HookManager::HookConfig config;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return config;
    }

    QTextStream in(&file);
    QString content = in.readAll();

    if (filePath.endsWith(".json")) {
        QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            config.name = obj.value("name").toString();
            config.type = hookTypeFromString(obj.value("type").toString());
            config.mode = obj.value("mode").toString() == "command" ? HookManager::HookMode::CommandBased : HookManager::HookMode::PromptBased;
            config.enabled = obj.value("enabled").toBool(true);
            config.command = obj.value("command").toString();
            config.args = obj.value("args").toVariant().toStringList();
            config.hookPrompt = obj.value("hookPrompt").toString();
            config.requiresLLMDecision = obj.value("requiresLLMDecision").toBool(false);
            config.timeout = obj.value("timeout").toInt(5000);
        }
    } else if (filePath.endsWith(".md")) {
        // 简化的 YAML frontmatter 解析
        QRegularExpression re("^---\\s*\\n(.*?)\\n---\\s*\\n", QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatch match = re.match(content);
        if (match.hasMatch()) {
            QString yaml = match.captured(1);
            // 这里为了简单，手动解析几行 YAML
            for (const QString& line : yaml.split('\n')) {
                QStringList parts = line.split(':');
                if (parts.size() >= 2) {
                    QString key = parts[0].trimmed();
                    QString value = parts[1].trimmed();
                    if (key == "name") config.name = value;
                    else if (key == "type") config.type = hookTypeFromString(value);
                    else if (key == "mode") config.mode = value == "command" ? HookManager::HookMode::CommandBased : HookManager::HookMode::PromptBased;
                    else if (key == "command") config.command = value;
                    else if (key == "enabled") config.enabled = (value == "true");
                }
            }
        }
    }

    if (config.name.isEmpty()) {
        config.name = QFileInfo(filePath).baseName();
    }

    return config;
}

bool saveHookToFile(const QString& filePath, const HookManager::HookConfig& config)
{
    // TODO: 实现保存为 Markdown + YAML frontmatter
    
    qInfo() << "[HookManager] Saving hook to file:" << filePath << "(stub)";
    
    return true;
}

void HookManager::setupDefaultSecurityRules()
{
    // 1. 阻止删除根目录或敏感目录
    HookConfig pathSecurity;
    pathSecurity.name = "path-protection";
    pathSecurity.type = HookType::PreToolUse;
    pathSecurity.mode = HookMode::CommandBased;
    pathSecurity.command = "sh";
    pathSecurity.args = QStringList() << "-c" <<
        "if echo \"$1\" | grep -qE '^(/etc/|/var/|/usr/|/bin/|/sbin/|/$)'; then "
        "  echo '{\"blockOperation\": true, \"systemMessage\": \"Access to system path blocked for security.\"}'; "
        "else "
        "  echo '{\"blockOperation\": false}'; "
        "fi" << "--";
    // 注意：这里需要传递工具输入中的路径，我们在 executeCommandHook 中会展开 ${tool_input.path} 等
    // 简化起见，这里先注册，具体逻辑可以通过脚本更精细实现
    registerHook(pathSecurity);

    // 2. 自动循环 (Ralph Wiggum 模式)
    // 当 Agent 想要退出但任务未完成时，拦截并提示继续
    HookConfig autoIterate;
    autoIterate.name = "ralph-wiggum";
    autoIterate.type = HookType::Stop;
    autoIterate.mode = HookMode::PromptBased;
    autoIterate.hookPrompt =
        "Evaluate if the task is truly finished. If there are pending steps or "
        "if the last output suggests more work is needed, return blockOperation: true "
        "and a system message explaining why we should continue.";
    autoIterate.requiresLLMDecision = true;
    autoIterate.enabled = false; // 默认禁用，用户按需开启
    registerHook(autoIterate);
}
