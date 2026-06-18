#include "agent/AgentEngine.h"
#include <QtConcurrent/QtConcurrent>
#include <QUuid>
#include <QEventLoop>
#include <QDebug>
#include <QRegularExpression>
#include <QMetaObject>
#include <QMutexLocker>
#include <QMutex>
#include <QJsonArray>

// Agent Runtime Enhancement components (Tier 3)
#include "agent/SlashCommandManager.h"
#include "agent/EventBus.h"
#include "agent/RuleEngine.h"
#include "agent/MCPManager.h"
#include "agent/ContextManager.h"
#include "agent/TaskOrchestrator.h"
#include "agent/ExecutionStrategyManager.h"
#include "agent/HookManager.h"
#include "agent/ErrorRecoveryManager.h"
#include "security/SecurityScanner.h"
#include "services/FileSnapshotService.h"

// Security components
#include "security/FolderTrustManager.h"

static const QString kDefaultSystem = R"(
You are NeurX Code, an expert software engineering AI assistant.
You have access to tools that let you read and write files, run shell commands,
and search the codebase. Use them to complete coding tasks accurately.

CRITICAL: All tools you have access to are REAL and FUNCTIONAL. When the user asks you
to create, edit, or write files, you MUST use the appropriate tool to actually perform
the operation. DO NOT just show code - EXECUTE the tool to create/modify files.
Your tools directly interact with the user's local filesystem and will create real files.
When describing your tools, do not invent "simulated environment", "theoretical only",
or "cannot execute in the real environment" limitations unless an actual runtime error
or approval policy has already produced that exact restriction.

Guidelines:
- IMPORTANT: When user asks to create/write a file, you MUST actually use the file writing tool to create it.
- Always read relevant files before making changes.
- Prefer targeted edits over rewriting entire files.
- Run tests after making changes when test tooling is available.
- Explain your reasoning briefly before each significant action.
- Ask for clarification if the task is ambiguous.
)";

static QString logMessagePreview(const QString &text, int maxLen = 120)
{
    const QString compact = text.simplified();
    if (compact.size() <= maxLen)
        return compact;
    return compact.left(maxLen) + QStringLiteral("...");
}

static QString previewRequestToolNames(const QJsonArray &tools, int maxCount = 8)
{
    QStringList names;
    const int limit = qMin(maxCount, tools.size());
    for (int i = 0; i < limit; ++i) {
        const QJsonObject tool = tools.at(i).toObject();
        const QJsonObject function = tool.value(QStringLiteral("function")).toObject();
        const QString name = function.value(QStringLiteral("name")).toString();
        if (!name.isEmpty())
            names << name;
    }
    if (tools.size() > limit)
        names << QStringLiteral("...+%1 more").arg(tools.size() - limit);
    return names.join(QStringLiteral(", "));
}

static QString previewToolArguments(const QJsonObject &args, int maxLen = 180)
{
    const QString json = QString::fromUtf8(QJsonDocument(args).toJson(QJsonDocument::Compact));
    return logMessagePreview(json, maxLen);
}

static QString askForApprovalToString(AskForApproval value)
{
    switch (value) {
    case AskForApproval::Never:
        return QStringLiteral("never");
    case AskForApproval::OnFailure:
        return QStringLiteral("on-failure");
    case AskForApproval::OnRequest:
        return QStringLiteral("on-request");
    case AskForApproval::Granular:
        return QStringLiteral("granular");
    case AskForApproval::UnlessTrusted:
        return QStringLiteral("unless-trusted");
    }
    return QStringLiteral("on-request");
}

static QString approvalReviewerToString(ApprovalsReviewer reviewer)
{
    switch (reviewer) {
    case ApprovalsReviewer::User:
        return QStringLiteral("user");
    case ApprovalsReviewer::AutoReview:
        return QStringLiteral("auto-review");
    case ApprovalsReviewer::Guardian:
        return QStringLiteral("guardian");
    }
    return QStringLiteral("user");
}

static QVariantMap approvalPolicyToVariantMap(const ApprovalPolicy &policy)
{
    QVariantMap map;
    map.insert(QStringLiteral("defaultPolicy"), askForApprovalToString(policy.defaultPolicy));
    map.insert(QStringLiteral("defaultReviewer"), approvalReviewerToString(policy.defaultReviewer));
    map.insert(QStringLiteral("requireNetworkApproval"), policy.requireNetworkApproval);
    map.insert(QStringLiteral("readOnlyMode"), policy.readOnlyMode);
    map.insert(QStringLiteral("autoApproveOnRetry"), policy.autoApproveOnRetry);

    QVariantList granularRules;
    for (const auto &rule : policy.granularRules) {
        granularRules.append(QVariantMap{
            {QStringLiteral("resourcePattern"), rule.resourcePattern},
            {QStringLiteral("approval"), askForApprovalToString(rule.approval)},
            {QStringLiteral("action"), rule.action},
            {QStringLiteral("toolNames"), rule.toolNames},
            {QStringLiteral("permanent"), rule.permanent},
        });
    }
    map.insert(QStringLiteral("granularRules"), granularRules);

    QVariantList restrictedProtocols;
    for (const auto &protocol : policy.restrictedProtocols) {
        restrictedProtocols.append(static_cast<int>(protocol));
    }
    map.insert(QStringLiteral("restrictedProtocols"), restrictedProtocols);
    map.insert(QStringLiteral("doubleConfirmPatterns"), policy.doubleConfirmPatterns);
    return map;
}

static QVariantMap buildApprovalProfile(const ApprovalManager *approvalManager,
                                        const ExecutionStrategyManager *strategyManager,
                                        bool autoApproveTools)
{
    QVariantMap profile;
    profile.insert(QStringLiteral("autoApproveTools"), autoApproveTools);

    if (approvalManager) {
        const ApprovalPolicy policy = approvalManager->getDefaultPolicy();
        profile.insert(QStringLiteral("approvalPolicy"), approvalPolicyToVariantMap(policy));
        profile.insert(QStringLiteral("readOnlyMode"), approvalManager->isReadOnlyMode());
        profile.insert(QStringLiteral("pendingApprovals"), approvalManager->getPendingApprovals().size());
    }

    if (strategyManager) {
        profile.insert(QStringLiteral("strategyStatistics"), strategyManager->getStatistics().toVariantMap());
        profile.insert(QStringLiteral("riskDistribution"), strategyManager->getRiskDistribution().toVariantMap());
        const ExecutionStrategy defaultStrategy = strategyManager->getStrategy(QStringLiteral("default"));
        if (!defaultStrategy.id.isEmpty()) {
            profile.insert(QStringLiteral("defaultStrategy"), QVariantMap{
                {QStringLiteral("id"), defaultStrategy.id},
                {QStringLiteral("name"), defaultStrategy.name},
                {QStringLiteral("description"), defaultStrategy.description},
                {QStringLiteral("approvalMode"), static_cast<int>(defaultStrategy.approvalMode)},
                {QStringLiteral("timeoutMs"), defaultStrategy.timeoutMs},
                {QStringLiteral("highRiskThreshold"), defaultStrategy.highRiskThreshold}
            });
        }
    }

    return profile;
}

static RiskAssessment assessToolCallRisk(const ToolCall &call, ExecutionStrategyManager *strategyManager)
{
    if (!strategyManager) {
        return {};
    }
    return strategyManager->assessToolRisk(call.name.trimmed(), call.arguments);
}

AgentEngine::AgentEngine(QObject *parent) : QObject(parent)
{
    m_config.systemPrompt = kDefaultSystem.trimmed();

    // Initialize Agent Runtime Enhancement managers (Tier 3)
    m_eventBus = std::make_unique<EventBus>(this);
    m_slashCommandManager = std::make_unique<SlashCommandManager>(this);
    m_ruleEngine = std::make_unique<RuleEngine>(this);
    m_mcpManager = std::make_unique<MCPManager>(this);
    m_contextManager = std::make_unique<ContextManager>(this);
    m_strategyManager = std::make_unique<ExecutionStrategyManager>(this);
    m_hookManager = std::make_unique<HookManager>(this);
    m_recoveryManager = std::make_unique<ErrorRecoveryManager>(this);
    m_securityScanner = std::make_unique<SecurityScanner>(this);
    m_taskOrchestrator = std::make_unique<TaskOrchestrator>(this);
    m_taskOrchestrator->setContextManager(m_contextManager.get());

    // Connect EventBus to important signals
    connect(this, &AgentEngine::toolExecuting, m_eventBus.get(), [this](const ToolCall &call) {
        m_eventBus->publishEvent(
            AgentEvent::Type::ToolCalled,
            call.name,
            call.arguments
        );
    });

    connect(this, &AgentEngine::toolFinished, m_eventBus.get(), [this](const ToolResult &result) {
        QJsonObject data;
        data["content"] = result.content;
        data["isError"] = result.isError;
        m_eventBus->publishEvent(
            result.isError ? AgentEvent::Type::ToolFailed : AgentEvent::Type::ToolCompleted,
            result.name,
            data
        );
    });

    connect(this, &AgentEngine::statusChanged, m_eventBus.get(), [this](AgentStatus status) {
        QString statusStr;
        switch (status) {
            case AgentStatus::Thinking:
                statusStr = "Thinking";
                break;
            case AgentStatus::Executing:
                statusStr = "Executing";
                break;
            case AgentStatus::Waiting:
                statusStr = "Waiting";
                break;
            case AgentStatus::Idle:
                statusStr = "Idle";
                break;
        }
        m_eventBus->publishCustomEvent(
            QStringLiteral("ExecutionStatusChanged"),
            QStringLiteral("AgentEngine"),
            QJsonObject{{"status", statusStr}}
        );
    });
}

AgentEngine::~AgentEngine() = default;

void AgentEngine::setProvider(LLMProvider *provider)
{
    if (m_provider) m_provider->disconnect(this);
    m_provider = provider;
    if (m_taskOrchestrator) {
        m_taskOrchestrator->setCurrentProvider(m_provider ? m_provider->providerId() : QString(),
                                               m_activeModel);
    }
    if (m_provider) {
        // Increase context budget for Gemini
        int budget = m_config.contextWindowTokens;
        if (m_provider->providerId() == "gemini") {
            budget = 1000000; // 1M tokens for Gemini
        }
        m_planner.setMaxTokens(m_config.maxCompletionTokens);
        m_planner.setContextBudget(qMax(0, budget - 8192));
        m_planner.setTemperature(0.0f);
        m_executor.setLLMProvider(m_provider);
    }
}

void AgentEngine::setToolRegistry(AgentToolRegistry *registry)
{
    m_registry = registry;
    m_executor.setToolRegistry(registry);
}

void AgentEngine::setApprovalManager(ApprovalManager *manager)
{
    m_approvalManager = manager;
}

void AgentEngine::setConfig(const AgentEngineConfig &config)
{
    m_config = config;
    m_planner.setMaxTokens(m_config.maxCompletionTokens);
    m_planner.setContextBudget(
        qMax(0, m_config.contextWindowTokens - 8192));
    m_planner.setTemperature(0.0f);
    m_planner.setSystemPrompt(m_config.systemPrompt);
}

void AgentEngine::setSystemPrompt(const QString &prompt)
{
    m_config.systemPrompt = prompt;
    m_planner.setSystemPrompt(prompt);
}

void AgentEngine::setAutoApproveTools(bool enabled)
{
    m_config.autoApproveTools = enabled;
}

void AgentEngine::setWorkspaceRoot(const QString &root)
{
    m_workspaceRoot = root;
    if (m_taskOrchestrator) {
        m_taskOrchestrator->setWorkspacePath(root);
    }
    m_folderTrustManager = FolderTrustManager::instance();
    if (m_folderTrustManager) {
        m_folderTrustManager->disconnect(this);
    }
    if (m_folderTrustManager && m_eventBus) {
        connect(m_folderTrustManager, &FolderTrustManager::trustDecisionRequired,
                this, [this](const QString &folderPath, const QStringList &items) {
            m_eventBus->publishCustomEvent(
                QStringLiteral("FolderTrustDecisionRequired"),
                QStringLiteral("FolderTrustManager"),
                QJsonObject{
                    {QStringLiteral("folder"), folderPath},
                    {QStringLiteral("items"), QJsonArray::fromStringList(items)}
                }
            );
        });

        connect(m_folderTrustManager, &FolderTrustManager::folderTrustChanged,
                this, [this](const QString &folderPath, bool trusted) {
            m_eventBus->publishCustomEvent(
                QStringLiteral("FolderTrustChanged"),
                QStringLiteral("FolderTrustManager"),
                QJsonObject{
                    {QStringLiteral("folder"), folderPath},
                    {QStringLiteral("trusted"), trusted}
                }
            );
        });
    }

    // Perform Folder Trust Discovery
    if (!m_workspaceRoot.isEmpty() && m_folderTrustManager) {
        // Check if folder is already trusted
        if (m_folderTrustManager->isFolderTrusted(root)) {
            qInfo() << "[AgentEngine] Workspace is trusted:" << root;
        } else {
            // Perform discovery to check for suspicious patterns
            bool isSafe = m_folderTrustManager->performTrustDiscovery(root);
            if (!isSafe) {
                qWarning() << "[AgentEngine] Suspicious content detected in workspace:" << root;
                // Emit signal to prompt user for trust decision
                QStringList suspiciousItems = m_folderTrustManager->scanForSuspiciousPatterns(root);
                if (!suspiciousItems.isEmpty()) {
                    if (m_eventBus) {
                        m_eventBus->publishEvent(
                            AgentEvent::Type::ExecutionFailed,
                            "SuspiciousContentDetected",
                            QJsonObject{{"folder", root}, {"items", QJsonArray::fromStringList(suspiciousItems)}}
                        );
                        m_eventBus->publishCustomEvent(
                            QStringLiteral("WorkspaceTrustRequired"),
                            QStringLiteral("AgentEngine"),
                            QJsonObject{
                                {QStringLiteral("folder"), root},
                                {QStringLiteral("items"), QJsonArray::fromStringList(suspiciousItems)}
                            }
                        );
                    }
                }
            } else {
                qInfo() << "[AgentEngine] Workspace passed trust discovery:" << root;
                // Auto-mark as trusted after successful discovery
                m_folderTrustManager->markFolderAsTrusted(root, "Auto-trusted after discovery");
            }
        }
    }
}

void AgentEngine::setActiveModel(const QString &model)
{
    if (m_activeModel == model) return;
    m_activeModel = model;
    if (m_taskOrchestrator) {
        m_taskOrchestrator->setCurrentProvider(m_provider ? m_provider->providerId() : QString(), model);
    }
    emit activeModelChanged(model);
}

QList<AgentMessage> AgentEngine::history() const
{
    QMutexLocker locker(&m_mutex);
    return m_history;
}

void AgentEngine::setHistory(const QList<AgentMessage> &history)
{
    QMutexLocker locker(&m_mutex);
    m_history = history;
}

void AgentEngine::clearHistory()
{
    QMutexLocker locker(&m_mutex);
    m_history.clear();
}

void AgentEngine::setStatus(AgentStatus s)
{
    if (m_status == s) return;
    m_status = s;
    emit statusChanged(s);
}

void AgentEngine::appendMessage(const AgentMessage &msg)
{
    qDebug() << "[AgentEngine::appendMessage]" << "role=" << (int)msg.role << "content=" << msg.content.left(50);
    {
        QMutexLocker locker(&m_mutex);
        m_history.append(msg);
    }
    emit messageAdded(msg);
}

QString AgentEngine::shellCommandFromCall(const ToolCall &call) const
{
    return call.arguments.value(QStringLiteral("command")).toString().trimmed();
}

bool AgentEngine::isDestructiveShellCommand(const QString &command) const
{
    const QString normalized = command.trimmed().toLower();
    if (normalized.isEmpty())
        return false;

    const QStringList patterns = {
        QStringLiteral(R"(\brm\b.*\s-rf\b)"),
        QStringLiteral(R"(\brm\b.*\s-rf\s)"),
        QStringLiteral(R"(\brm\b.*\s--recursive\b)"),
        QStringLiteral(R"(\bgit\b.*\breset\b.*\b--hard\b)"),
        QStringLiteral(R"(\bgit\b.*\bclean\b.*\b-f\b)"),
        QStringLiteral(R"(\bchmod\b.*\b-R\b.*\b777\b)"),
        QStringLiteral(R"(\bchown\b.*\b-R\b)"),
        QStringLiteral(R"(\bdd\b.*\bof=/dev/\w+\b)"),
        QStringLiteral(R"(\bmkfs\w*\b)"),
        QStringLiteral(R"(\bshutdown\b|\breboot\b|\bhalt\b)"),
        QStringLiteral(R"(\bpowershell\b.*\bremove-item\b.*\b-recurse\b)"),
        QStringLiteral(R"(\bdel\b.*\s\/s\b.*\s\/q\b)")
    };

    for (const auto &pattern : patterns) {
        const QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
        if (re.match(normalized).hasMatch())
            return true;
    }

    return false;
}

QString AgentEngine::approvalRiskLevel(const ToolCall &call) const
{
    const QString toolName = call.name.trimmed();
    if (m_strategyManager) {
        const RiskAssessment assessment = assessToolCallRisk(call, m_strategyManager.get());
        if (!assessment.level.isEmpty()) {
            return assessment.level;
        }
    }

    if (toolName == QStringLiteral("run_command") || toolName == QStringLiteral("run_docker_command")) {
        const QString command = shellCommandFromCall(call);
        if (isDestructiveShellCommand(command))
            return QStringLiteral("critical");
        if (toolName == QStringLiteral("run_docker_command"))
            return QStringLiteral("low"); // Sandbox is safer
        return QStringLiteral("high");
    }

    if (toolName == QStringLiteral("patch")
        || toolName == QStringLiteral("apply_patch")
        || toolName == QStringLiteral("file_system")
        || toolName == QStringLiteral("codex_file_system")
        || toolName == QStringLiteral("file_creation")
        || toolName == QStringLiteral("github")
        || toolName == QStringLiteral("gitlab")
        || toolName == QStringLiteral("jira"))
        return QStringLiteral("high");

    if (toolName == QStringLiteral("web_search")
        || toolName == QStringLiteral("google_search")
        || toolName == QStringLiteral("web_fetch"))
        return QStringLiteral("medium");

    if (toolName == QStringLiteral("codex_agent")) {
        if (!call.arguments.value(QStringLiteral("file_path")).toString().trimmed().isEmpty()
            || !call.arguments.value(QStringLiteral("new_text")).toString().isEmpty()) {
            return QStringLiteral("high");
        }
        return QStringLiteral("medium");
    }

    return QStringLiteral("low");
}

QString AgentEngine::approvalResourceForCall(const ToolCall &call) const
{
    const QString toolName = call.name.trimmed();
    if (toolName == QStringLiteral("run_command") || toolName == QStringLiteral("run_docker_command"))
        return shellCommandFromCall(call);
    if (toolName == QStringLiteral("patch") || toolName == QStringLiteral("apply_patch")) {
        const QString patch = call.arguments.value(QStringLiteral("patch")).toString();
        return patch.isEmpty() ? call.arguments.value(QStringLiteral("input")).toString() : patch;
    }
    if (toolName == QStringLiteral("github") || toolName == QStringLiteral("gitlab") || toolName == QStringLiteral("jira"))
        return QStringLiteral("%1 %2").arg(call.arguments.value("action").toString(), call.arguments.value("repo").toString() + call.arguments.value("project").toString() + call.arguments.value("issue_key").toString());
    if (toolName == QStringLiteral("file_system")
        || toolName == QStringLiteral("codex_file_system")) {
        const QString op = call.arguments.value(QStringLiteral("operation")).toString();
        const QString path = call.arguments.value(QStringLiteral("path")).toString();
        const QString destination = call.arguments.value(QStringLiteral("destination")).toString();
        return destination.isEmpty()
            ? QStringLiteral("%1 %2").arg(op, path)
            : QStringLiteral("%1 %2 -> %3").arg(op, path, destination);
    }
    if (toolName == QStringLiteral("file_creation")) {
        const QString op = call.arguments.value(QStringLiteral("operation")).toString();
        const QString path = call.arguments.value(QStringLiteral("path")).toString();
        return QStringLiteral("%1 %2").arg(op, path);
    }
    if (toolName == QStringLiteral("codex_agent")) {
        const QString filePath = call.arguments.value(QStringLiteral("file_path")).toString();
        return !filePath.trimmed().isEmpty()
            ? QStringLiteral("write %1").arg(filePath)
            : call.arguments.value(QStringLiteral("task")).toString();
    }
    return QString();
}

bool AgentEngine::shouldRequireApproval(const ToolCall &call) const
{
    const QString toolName = call.name.trimmed();
    const QString resource = approvalResourceForCall(call);
    bool requireApproval = false;
    bool explicitAllow = false;

    if (m_approvalManager) {
        const AskForApproval policy = m_approvalManager->getPolicyFor(toolName, resource);
        if (policy == AskForApproval::Never) {
            explicitAllow = true;
        }
        if (policy == AskForApproval::OnRequest
            || policy == AskForApproval::Granular
            || policy == AskForApproval::UnlessTrusted) {
            requireApproval = true;
        }
    }

    if (!requireApproval && m_strategyManager) {
        const ExecutionStrategy strategy = (toolName == QStringLiteral("run_command")
                                            || toolName == QStringLiteral("run_docker_command"))
            ? m_strategyManager->getStrategyForCommand(shellCommandFromCall(call))
            : m_strategyManager->getStrategyForTool(toolName);
        const RiskAssessment risk = assessToolCallRisk(call, m_strategyManager.get());
        requireApproval = m_strategyManager->needsApproval(risk, strategy);
    }

    if (requireApproval)
        return true;

    if (explicitAllow)
        return false;

    if (!m_config.autoApproveTools)
        return true;

    const QString risk = approvalRiskLevel(call);
    return risk == QStringLiteral("high") || risk == QStringLiteral("critical");
}

void AgentEngine::injectContext(const QString &filePath, const QString &content,
                                int startLine, int endLine)
{
    QString ctx = QString("```\n// File: %1").arg(filePath);
    if (startLine > 0) ctx += QString(" (lines %1-%2)").arg(startLine).arg(endLine);
    ctx += "\n" + content + "\n```";

    if (m_contextManager) {
        ContextItem item;
        item.type = QStringLiteral("file");
        item.source = filePath;
        item.content = content;
        item.transient = true;
        item.priority = 55;
        item.cacheable = true; // Injecting a file is a good breakpoint for caching
        item.metadata["display"] = ctx;
        if (startLine > 0) item.metadata["startLine"] = startLine;
        if (endLine > 0) item.metadata["endLine"] = endLine;
        m_contextManager->addContextItem(item);
    }
    if (m_taskOrchestrator) {
        m_taskOrchestrator->recordContextSnapshot(QStringLiteral("injectContext"));
    }

    AgentMessage msg;
    msg.role    = MessageRole::User;
    msg.content = ctx;
    msg.cacheable = true; // Mark context injection as cacheable
    appendMessage(msg);
}

void AgentEngine::submitUserMessage(const QString &text, const QVariantList &attachments)
{
    if (m_status != AgentStatus::Idle) return;

    // Check for slash commands (Tier 3 enhancement)
    if (text.startsWith('/') && m_slashCommandManager) {
        if (m_taskOrchestrator && m_taskOrchestrator->currentThreadId().isEmpty()) {
            TaskOrchestrator::StartOptions options;
            options.workspacePath = m_workspaceRoot;
            options.currentProvider = m_provider ? m_provider->providerId() : QString();
            options.currentModel = m_activeModel;
            options.contextItems = m_contextManager ? m_contextManager->exportContextItems() : QVariantList{};
            options.approvalProfile = buildApprovalProfile(m_approvalManager, m_strategyManager.get(), m_config.autoApproveTools);
            m_taskOrchestrator->startTask(text, options);
        }

        QJsonObject slashContext;
        slashContext["workspaceRoot"] = m_workspaceRoot;
        slashContext["activeModel"] = m_activeModel;
        slashContext["historySize"] = static_cast<int>(m_history.size());
        if (m_contextManager) {
            slashContext["contextItems"] = m_contextManager->getContextAsJSON();
            slashContext["contextText"] = m_contextManager->getContextAsText();
            slashContext["contextSize"] = m_contextManager->getContextSize();
            slashContext["contextCount"] = static_cast<int>(m_contextManager->allContextItems().size());
        }

        SlashCommandResult result = m_slashCommandManager->executeCommand(text, slashContext);
        if (result.success) {
            if (m_taskOrchestrator) {
                m_taskOrchestrator->recordUserMessage(text, attachments);
            }

            AgentMessage cmdResponse;
            cmdResponse.role = MessageRole::Assistant;
            cmdResponse.content = result.output;
            appendMessage(cmdResponse);
            if (m_taskOrchestrator) {
                m_taskOrchestrator->recordAssistantMessage(cmdResponse);
            }

            // If the command specified an agent, we might want to switch or delegate
            if (result.metadata.contains("agent")) {
                // Future: implementation for agent delegation
                qDebug() << "[AgentEngine] Command requested specialized agent:" << result.metadata["agent"].toString();
            }

            emit turnComplete();
            return;
        }
    }

    AgentMessage userMsg;
    userMsg.role    = MessageRole::User;
    userMsg.content = text;
    userMsg.attachments = attachments;

    if (m_taskOrchestrator && m_taskOrchestrator->currentThreadId().isEmpty()) {
        TaskOrchestrator::StartOptions options;
        options.workspacePath = m_workspaceRoot;
        options.currentProvider = m_provider ? m_provider->providerId() : QString();
        options.currentModel = m_activeModel;
        options.contextItems = m_contextManager ? m_contextManager->exportContextItems() : QVariantList{};
        options.approvalProfile = buildApprovalProfile(m_approvalManager, m_strategyManager.get(), m_config.autoApproveTools);
        m_taskOrchestrator->startTask(text, options);
    }

    if (m_taskOrchestrator) {
        m_taskOrchestrator->recordUserMessage(text, attachments);
    }

    appendMessage(userMsg);

    m_interrupted = false;
    setStatus(AgentStatus::Thinking);

    const auto future = QtConcurrent::run([this]() { runLoop(); });
    Q_UNUSED(future);
}

void AgentEngine::interrupt() { m_interrupted = true; }

void AgentEngine::approveTool(const QString &callId, bool approved)
{
    QMutexLocker locker(&m_mutex);
    if (!m_pendingApprovals.contains(callId)) return;
    if (!approved) {
        m_pendingApprovals.remove(callId);
        ToolResult denied{callId, "", true, "Tool execution denied by user."};
        AgentMessage resultMsg;
        resultMsg.role = MessageRole::Tool;
        resultMsg.toolResults.append(denied);
        if (m_taskOrchestrator) {
            m_taskOrchestrator->recordToolResult(denied);
        }
        locker.unlock();
        appendMessage(resultMsg);
        return;
    }

    // Approval must remove the pending entry so the waiting runLoop can continue.
    m_pendingApprovals.remove(callId);
}

void AgentEngine::runLoop()
{
    int iterations = 0;

    // Session Start Hooks
    if (m_hookManager) {
        QString hookPrompt = m_hookManager->getSessionStartPrompt();
        if (!hookPrompt.isEmpty()) {
            AgentMessage msg;
            msg.role = MessageRole::System;
            msg.content = "System Instructions from Hooks:\n" + hookPrompt;
            appendMessage(msg);
        }
    }

    while (iterations++ < m_config.maxIterations && !m_interrupted) {
        if (!m_provider) {
            emit errorOccurred("No LLM provider configured.");
            break;
        }

        // ── Plan request ─────────────────────────────────────────────────────
        const QString providerId = m_provider ? m_provider->providerId() : QString{};
        QList<AgentMessage> requestHistory = history();
        if (m_contextManager) {
            const QString contextText = m_contextManager->getContextAsText().trimmed();
            if (!contextText.isEmpty()) {
                AgentMessage contextMsg;
                contextMsg.role = MessageRole::System;
                contextMsg.content = QStringLiteral(
                    "Workspace context snapshot:\n"
                ) + contextText;
                requestHistory.prepend(contextMsg);
            }
        }
        const LLMRequest req = m_planner.buildRequest(
            requestHistory,
            m_activeModel,
            providerId,
            m_registry);

        qInfo().noquote() << "[agent] request start:"
                  << "provider=" << providerId
                  << "model=" << (req.model.isEmpty() ? m_activeModel : req.model)
                  << "iteration=" << iterations
                  << "messages=" << req.messages.size()
                  << "tools=" << req.tools.size()
                  << "toolNames=[" << previewRequestToolNames(req.tools) << "]";

        setStatus(AgentStatus::Thinking);

        LLMResponse response;
        bool        done = false;
        QString     providerError;

        // Connect provider signals for this request.
        QMetaObject::Connection connToken = connect(
            m_provider, &LLMProvider::tokenReceived,
            this, [this](const TokenEvent &ev) { emit tokenReceived(ev); });

        QMetaObject::Connection connComplete = connect(
            m_provider, &LLMProvider::responseComplete,
            this, [&](const LLMResponse &r) { response = r; done = true; });

        QMetaObject::Connection connError = connect(
            m_provider, &LLMProvider::requestError,
            this, [&](const QString &err) { providerError = err; done = true; });

        // Run the network request on the provider's own thread. The providers
        // own QNetworkAccessManager instances, so calling them from the worker
        // thread trips Qt's cross-thread QObject checks.
        QEventLoop loop;
        connect(m_provider, &LLMProvider::responseComplete, &loop, &QEventLoop::quit);
        connect(m_provider, &LLMProvider::requestError,     &loop, &QEventLoop::quit);
        if (m_provider->thread() == QThread::currentThread()) {
            m_provider->sendRequest(req);
        } else {
            QMetaObject::invokeMethod(
                m_provider,
                [provider = m_provider, req]() { provider->sendRequest(req); },
                Qt::QueuedConnection);
        }
        loop.exec();

        disconnect(connToken);
        disconnect(connComplete);
        disconnect(connError);

        if (!providerError.isEmpty()) {
            qWarning().noquote() << "[agent] request failed:" << providerError;
            emit errorOccurred(providerError);
            break;
        }

        qInfo().noquote() << "[agent] response received:"
                          << "content=" << logMessagePreview(response.message.content)
                          << "toolCalls=" << response.message.toolCalls.size();
        for (const ToolCall &call : response.message.toolCalls) {
            qInfo().noquote() << "[agent] tool call:"
                              << "name=" << call.name
                              << "id=" << call.id
                              << "args=" << previewToolArguments(call.arguments);
        }

        appendMessage(response.message);
        if (m_taskOrchestrator) {
            m_taskOrchestrator->recordAssistantMessage(response.message);
        }

        // ── No tool calls → turn is complete ─────────────────────────────────
        if (m_verifier.turnComplete(response.message)) {
            break;
        }

        // ── Execute tool calls ────────────────────────────────────────────────
        setStatus(AgentStatus::Executing);
        AgentMessage resultsMsg;
        resultsMsg.role = MessageRole::Tool;

        for (const auto &call : response.message.toolCalls) {
            if (m_interrupted) break;

            // Capture state before potentially destructive operations
            QString checkpointId;
            if (m_recoveryManager && (call.name.contains("write") || call.name.contains("patch") || call.name.contains("delete"))) {
                QStringList targetFiles;
                if (call.arguments.contains("path")) targetFiles << call.arguments["path"].toString();
                if (call.arguments.contains("file_path")) targetFiles << call.arguments["file_path"].toString();

                if (!targetFiles.isEmpty()) {
                    QString snapshotId = FileSnapshotService::instance()->takeSnapshot(targetFiles);
                    checkpointId = m_recoveryManager->createCheckpoint(call.id, QString("Pre-tool: %1").arg(call.name));
                    QJsonObject state;
                    state["fileSnapshotId"] = snapshotId;
                    m_recoveryManager->saveCheckpoint(checkpointId, state);
                    qInfo() << "[AgentEngine] Created recovery checkpoint:" << checkpointId << "for tool:" << call.name;
                }
            }

            // Hook: PreToolUse
            if (m_hookManager && !m_hookManager->shouldAllowToolUse(call.name, call.arguments)) {
                qWarning() << "[AgentEngine] Tool execution blocked by hook:" << call.name;
                ToolResult blockedResult{call.id, call.name, true, "Execution blocked by security policy (Hook)."};
                if (m_taskOrchestrator) {
                    m_taskOrchestrator->recordToolCall(call, QStringLiteral("blocked_by_hook"));
                    m_taskOrchestrator->recordToolResult(blockedResult);
                }
                resultsMsg.toolResults.append(blockedResult);
                emit toolFinished(blockedResult);
                continue;
            }

            // 🛡️ Security Check: Pattern scanning for file modifications
            if (m_securityScanner && (call.name == "patch" || call.name == "apply_patch" || call.name == "write_file" || call.name.contains("edit"))) {
                QString contentToCheck;
                if (call.arguments.contains("content")) contentToCheck = call.arguments["content"].toString();
                else if (call.arguments.contains("patch")) contentToCheck = call.arguments["patch"].toString();
                else if (call.arguments.contains("text")) contentToCheck = call.arguments["text"].toString();

                if (!contentToCheck.isEmpty()) {
                    auto issues = m_securityScanner->scanContent(contentToCheck, call.arguments["path"].toString());
                    if (!issues.isEmpty()) {
                        QString warning = "⚠️ Security Warning: Dangerous patterns detected in your suggested change:\n";
                        for (const auto& issue : issues) {
                            warning += QString("- [%1] %2 (%3)\n").arg(severityToString(issue.severity), issue.message, issue.pattern);
                        }
                        warning += "\nPlease revise the code to follow security best practices.";

                        qWarning() << "[AgentEngine] Security issues found in tool call:" << call.name;

                        ToolResult securityResult{call.id, call.name, true, warning};
                        if (m_taskOrchestrator) {
                            m_taskOrchestrator->recordToolResult(securityResult);
                        }
                        resultsMsg.toolResults.append(securityResult);
                        emit toolFinished(securityResult);
                        continue;
                    }
                }
            }

            if (shouldRequireApproval(call)) {
                if (m_taskOrchestrator) {
                    m_taskOrchestrator->recordToolCall(call, QStringLiteral("approval_required"));
                }
                {
                    QMutexLocker locker(&m_mutex);
                    m_pendingApprovals[call.id] = call;
                }
                emit toolApprovalRequired(call, approvalRiskLevel(call));
                setStatus(AgentStatus::Waiting);
                // Wait for approval
                bool pending = true;
                while (pending && !m_interrupted) {
                    QThread::msleep(100);
                    QMutexLocker locker(&m_mutex);
                    pending = m_pendingApprovals.contains(call.id);
                }
                setStatus(AgentStatus::Executing);
                if (m_interrupted) break;
            }

            if (m_taskOrchestrator) {
                m_taskOrchestrator->recordToolCall(call, QStringLiteral("started"));
            }
            emit toolExecuting(call);
            // Forward streaming chunks from the tool to our own signal so the
            // main-thread UI can update the tool card live.
            QMetaObject::Connection streamConn;
            if (auto *t = m_registry->tool(call.name))
                streamConn = connect(t, &BaseTool::outputChunk,
                                     this, &AgentEngine::toolOutputChunk,
                                     Qt::DirectConnection);
            const ToolResult result = m_executor.execute(call);
            if (streamConn) disconnect(streamConn);

            // If tool failed, consider automatic recovery
            if (result.isError && m_recoveryManager && !checkpointId.isEmpty()) {
                qWarning() << "[AgentEngine] Tool failed, checking recovery for:" << call.name;
                // For now, we don't auto-rollback unless it's a critical strategy,
                // but we make the checkpoint available.
            }

            qInfo().noquote() << "[agent] tool result:"
                              << call.name
                              << "callId=" << call.id
                              << "error=" << (result.isError ? "true" : "false");
            if (m_taskOrchestrator) {
                m_taskOrchestrator->recordToolResult(result);
            }
            emit toolFinished(result);
            resultsMsg.toolResults.append(result);

            // Hook: PostToolUse
            if (m_hookManager) {
                QJsonObject postContext;
                postContext["tool"] = call.name;
                postContext["isError"] = result.isError;
                postContext["content"] = result.content;
                m_hookManager->executeHooks(HookManager::HookType::PostToolUse, postContext);
            }
        }

        appendMessage(resultsMsg);

        // Hook: Stop (Autonomous iteration)
        if (m_hookManager && iterations < m_config.maxIterations) {
            QJsonObject stopContext;
            stopContext["iterations"] = iterations;
            stopContext["last_message"] = response.message.content;

            if (m_hookManager->shouldContinueSession(stopContext)) {
                qInfo() << "[AgentEngine] Hook requested to continue session (Ralph Wiggum mode)";
                continue; // Force another iteration
            }
        }
    }

    setStatus(AgentStatus::Idle);
    emit turnComplete();
}

// ────────────────────────────────────────────────────────────────────────────
// Folder Trust Discovery Implementation
// ────────────────────────────────────────────────────────────────────────────

bool AgentEngine::isFolderTrusted(const QString &folder) const
{
    if (!m_folderTrustManager) {
        return false;
    }
    return m_folderTrustManager->isFolderTrusted(folder);
}

void AgentEngine::markFolderAsTrusted(const QString &folder, const QString &reason)
{
    if (!m_folderTrustManager) {
        m_folderTrustManager = FolderTrustManager::instance();
    }
    m_folderTrustManager->markFolderAsTrusted(folder, reason);
    qInfo() << "[AgentEngine] Folder marked as trusted:" << folder << "(" << reason << ")";
}

void AgentEngine::markFolderAsUntrusted(const QString &folder)
{
    if (!m_folderTrustManager) {
        m_folderTrustManager = FolderTrustManager::instance();
    }
    m_folderTrustManager->markFolderAsUntrusted(folder);
    qWarning() << "[AgentEngine] Folder marked as untrusted:" << folder;
}

FolderTrustManager *AgentEngine::folderTrustManager() const
{
    // Return the singleton instance if available
    if (m_folderTrustManager) {
        return m_folderTrustManager;
    }
    // Return the singleton instance directly
    return FolderTrustManager::instance();
}
