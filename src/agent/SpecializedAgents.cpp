#include "SpecializedAgents.h"
#include <QDebug>
#include <QDateTime>
#include <QtConcurrent>
#include <QSemaphore>
#include <QFuture>
#include <QFutureWatcher>

namespace neurx {

SpecializedAgent::SpecializedAgent(const AgentConfig& config, QObject* parent)
    : QObject(parent), m_config(config), m_llmProvider(nullptr), m_toolRegistry(nullptr)
{
}

// CodeExplorerAgent implementation
CodeExplorerAgent::CodeExplorerAgent(QObject* parent)
    : SpecializedAgent(AgentConfig{
        "code-explorer",
        "Code Explorer",
        "Expert in exploring and analyzing codebases",
        AgentExpertise::CodeExploration,
        "You are an expert in codebase exploration...",
        "gpt-4o",
        0.2,
        2000,
        128000,
        {"search", "read_file", "list_dir"},
        {},
        {},
        true,
        120,
        2,
        true,
        "What part of the code should I explore?",
        {},
        {}
    }, parent)
{
}

void CodeExplorerAgent::executeTask(const AgentTask& task,
                                   std::function<void(const AgentResult&)> callback)
{
    // Basic implementation
    AgentResult result;
    result.success = true;
    result.taskId = task.taskId;
    result.agentId = id();
    result.result = "Exploring code for: " + task.query;
    callback(result);
}

void CodeExplorerAgent::cancelTask(const QString& taskId) {}

// CodeArchitectAgent implementation
CodeArchitectAgent::CodeArchitectAgent(QObject* parent)
    : SpecializedAgent(AgentConfig{
        "code-architect",
        "Code Architect",
        "Expert in system design and architecture",
        AgentExpertise::Architecture,
        "You are an expert software architect...",
        "gpt-4o",
        0.3,
        4000,
        128000,
        {"read_file", "list_dir"},
        {},
        {},
        false,
        300,
        1,
        true,
        "What feature should I design?",
        {},
        {}
    }, parent)
{
}

void CodeArchitectAgent::executeTask(const AgentTask& task,
                                    std::function<void(const AgentResult&)> callback)
{
    AgentResult result;
    result.success = true;
    result.taskId = task.taskId;
    result.agentId = id();
    result.result = "Designing architecture for: " + task.query;
    callback(result);
}

void CodeArchitectAgent::cancelTask(const QString& taskId) {}

// CodeReviewerAgent implementation
CodeReviewerAgent::CodeReviewerAgent(QObject* parent)
    : SpecializedAgent(AgentConfig{
        "code-reviewer",
        "Code Reviewer",
        "Expert in code quality and security review",
        AgentExpertise::CodeReview,
        "You are an expert code reviewer...",
        "gpt-4o",
        0.2,
        2000,
        128000,
        {"read_file", "diff_tool"},
        {},
        {},
        true,
        180,
        2,
        true,
        "What code should I review?",
        {},
        {}
    }, parent)
{
}

void CodeReviewerAgent::executeTask(const AgentTask& task,
                                   std::function<void(const AgentResult&)> callback)
{
    AgentResult result;
    result.success = true;
    result.taskId = task.taskId;
    result.agentId = id();
    result.result = "Reviewing code for: " + task.query;
    callback(result);
}

void CodeReviewerAgent::cancelTask(const QString& taskId) {}

// TestAnalyzerAgent implementation
TestAnalyzerAgent::TestAnalyzerAgent(QObject* parent)
    : SpecializedAgent(AgentConfig{
        "test-analyzer",
        "Test Analyzer",
        "Expert in testing and validation",
        AgentExpertise::Testing,
        "You are an expert in software testing...",
        "gpt-4o",
        0.2,
        2000,
        128000,
        {"read_file", "run_tests"},
        {},
        {},
        true,
        240,
        2,
        true,
        "What should I test?",
        {},
        {}
    }, parent)
{
}

void TestAnalyzerAgent::executeTask(const AgentTask& task,
                                   std::function<void(const AgentResult&)> callback)
{
    AgentResult result;
    result.success = true;
    result.taskId = task.taskId;
    result.agentId = id();
    result.result = "Analyzing tests for: " + task.query;
    callback(result);
}

void TestAnalyzerAgent::cancelTask(const QString& taskId) {}

// WebTestingAgent implementation
WebTestingAgent::WebTestingAgent(QObject* parent)
    : SpecializedAgent(AgentConfig{
        "web-tester",
        "Web Tester",
        "Expert in web application testing using Playwright",
        AgentExpertise::Testing,
        "You are an expert in web application testing. Use Playwright to interact with local web apps.\n\n"
        "Follow the 'Reconnaissance-then-action' pattern:\n"
        "1. Start the server (if needed) using 'scripts/with_server.py'\n"
        "2. Navigate to the app and wait for 'networkidle'\n"
        "3. Take screenshots or inspect the DOM to identify selectors\n"
        "4. Execute automation logic with discovered selectors\n\n"
        "Always run scripts with --help first. Use sync_playwright() and always close the browser.",
        "gpt-4o",
        0.2,
        4000,
        128000,
        {"run_script", "read_file", "list_dir"},
        {},
        {},
        true,
        300,
        2,
        true,
        "Which web app should I test?",
        {},
        {{"helper_script", "scripts/with_server.py"}}
    }, parent)
{
}

void WebTestingAgent::executeTask(const AgentTask& task,
                                 std::function<void(const AgentResult&)> callback)
{
    AgentResult result;
    result.success = true;
    result.taskId = task.taskId;
    result.agentId = id();
    result.result = "Testing web app with Playwright: " + task.query;
    callback(result);
}

void WebTestingAgent::cancelTask(const QString& taskId) {}

// DocCoauthoringAgent implementation
DocCoauthoringAgent::DocCoauthoringAgent(QObject* parent)
    : SpecializedAgent(AgentConfig{
        "doc-coauthor",
        "Doc Co-author",
        "Expert in structured document co-authoring",
        AgentExpertise::Documentation,
        "You are an expert documentation guide. Follow the three-stage workflow:\n\n"
        "STAGE 1: Context Gathering (understand audience, goals, and gather meta-context)\n"
        "STAGE 2: Refinement & Structure (build section-by-section through brainstorming and curation)\n"
        "STAGE 3: Reader Testing (verify with a fresh Claude sub-agent to catch blind spots)\n\n"
        "Act as an active guide, walking the user through these stages. For Reader Testing, "
        "always use a fresh sub-agent without the current conversation context.",
        "gpt-4o",
        0.3,
        4000,
        128000,
        {"read_file", "write_file"},
        {},
        {},
        false,
        600,
        1,
        true,
        "What document are we writing today?",
        {},
        {}
    }, parent)
{
}

void DocCoauthoringAgent::executeTask(const AgentTask& task,
                                     std::function<void(const AgentResult&)> callback)
{
    AgentResult result;
    result.success = true;
    result.taskId = task.taskId;
    result.agentId = id();
    result.result = "Co-authoring document: " + task.query;
    callback(result);
}

void DocCoauthoringAgent::cancelTask(const QString& taskId) {}

// AlgorithmicArtAgent implementation
AlgorithmicArtAgent::AlgorithmicArtAgent(QObject* parent)
    : SpecializedAgent(AgentConfig{
        "art-creator",
        "Algorithmic Art Creator",
        "Expert in creating generative art using p5.js",
        AgentExpertise::General,
        "You are an expert in computational aesthetics. Follow this two-step process:\n\n"
        "1. ALGORITHMIC PHILOSOPHY CREATION: Create a 4-6 paragraph manifesto (.md) for a generative movement. "
        "Focus on emergent behavior, seeded randomness, and master-level craftsmanship.\n"
        "2. P5.JS IMPLEMENTATION: Express the philosophy through code. Use 'templates/viewer.html' as the foundation. "
        "Keep Anthropic branding and seed controls fixed; customize the algorithm and parameters.\n\n"
        "Always use seeded randomness (randomSeed/noiseSeed) for reproducibility.",
        "gpt-4o",
        0.7,
        4000,
        128000,
        {"write_file"},
        {},
        {},
        false,
        300,
        1,
        true,
        "What kind of algorithmic art should I create?",
        {},
        {{"template", "templates/viewer.html"}}
    }, parent)
{
}

void AlgorithmicArtAgent::executeTask(const AgentTask& task,
                                     std::function<void(const AgentResult&)> callback)
{
    AgentResult result;
    result.success = true;
    result.taskId = task.taskId;
    result.agentId = id();
    result.result = "Creating algorithmic art: " + task.query;
    callback(result);
}

void AlgorithmicArtAgent::cancelTask(const QString& taskId) {}

// AgentOrchestrator implementation
AgentOrchestrator::AgentOrchestrator(QObject* parent)
    : QObject(parent), m_llmProvider(nullptr), m_toolRegistry(nullptr)
{
    // Register default agents
    registerAgent(std::make_shared<CodeExplorerAgent>(this));
    registerAgent(std::make_shared<CodeArchitectAgent>(this));
    registerAgent(std::make_shared<CodeReviewerAgent>(this));
    registerAgent(std::make_shared<TestAnalyzerAgent>(this));
    registerAgent(std::make_shared<WebTestingAgent>(this));
    registerAgent(std::make_shared<DocCoauthoringAgent>(this));
    registerAgent(std::make_shared<AlgorithmicArtAgent>(this));
}

void AgentOrchestrator::registerAgent(std::shared_ptr<SpecializedAgent> agent)
{
    if (agent) {
        m_agents[agent->id()] = agent;
        emit agentRegistered(agent->id());
    }
}

void AgentOrchestrator::unregisterAgent(const QString& agentId)
{
    if (m_agents.remove(agentId)) {
        emit agentUnregistered(agentId);
    }
}

std::shared_ptr<SpecializedAgent> AgentOrchestrator::getAgent(const QString& agentId) const
{
    return m_agents.value(agentId);
}

QList<AgentConfig> AgentOrchestrator::getAllAgents() const
{
    QList<AgentConfig> configs;
    for (const auto& agent : m_agents) {
        configs.append(agent->config());
    }
    return configs;
}

void AgentOrchestrator::executeTask(const AgentTask& task,
                                   std::function<void(const AgentResult&)> callback)
{
    auto agent = getAgent(task.agentId);
    if (agent) {
        emit taskStarted(task.taskId, task.agentId);
        agent->executeTask(task, [this, task, callback](const AgentResult& result) {
            emit taskCompleted(task.taskId, result.success);
            callback(result);
        });
    } else {
        AgentResult result;
        result.success = false;
        result.taskId = task.taskId;
        result.error = "Agent not found: " + task.agentId;
        callback(result);
    }
}

void AgentOrchestrator::executeParallel(const QList<AgentTask>& tasks,
                        std::function<void(const QList<AgentResult>&)> callback)
{
    // Improved parallel execution using QtConcurrent with per-task synchronization.
    // For each task we call executeTask(agent, callback) and block inside a worker
    // thread until the agent's callback is invoked or the task's timeout elapses.

    QVector<QFuture<AgentResult>> futures;
    futures.reserve(tasks.size());

    for (const auto &task : tasks) {
        // Capture by value for thread-safety
        AgentTask t = task;

        QFuture<AgentResult> f = QtConcurrent::run([this, t]() -> AgentResult {
            AgentResult result;
            QSemaphore sem(0);

            // Use the agent if present
            auto agent = getAgent(t.agentId);
            if (!agent) {
                result.success = false;
                result.taskId = t.taskId;
                result.agentId = t.agentId;
                result.error = "Agent not found: " + t.agentId;
                return result;
            }

            // Call executeTask and wait for the callback to be invoked.
            // The agent implementation is expected to invoke the callback when done.
            agent->executeTask(t, [&result, &sem](const AgentResult &r) {
                result = r;
                sem.release(1);
            });

            // Wait for callback or timeout
            int waitMs = t.timeoutMs > 0 ? t.timeoutMs : 300000; // default 5min
            bool acquired = sem.tryAcquire(1, waitMs);
            if (!acquired) {
                // Timeout: attempt to cancel the agent task if the agent supports it
                try {
                    if (agent) {
                        agent->cancelTask(t.taskId);
                    }
                } catch (...) {
                    // swallow exceptions; agent cancel may be no-op
                }

                result.success = false;
                result.taskId = t.taskId;
                result.agentId = t.agentId;
                result.error = QString("Agent task timed out after %1 ms").arg(waitMs);
            }

            return result;
        });

        futures.append(f);
    }

    // Wait for all futures to finish and collect results.
    QList<AgentResult> results;
    for (auto &f : futures) {
        f.waitForFinished();
        results.append(f.result());
    }

    if (callback) callback(results);
}

void AgentOrchestrator::executeSequential(const QList<AgentTask>& tasks,
                                         std::function<void(const QList<AgentResult>&)> callback)
{
    // Simplified sequential execution
    auto results = std::make_shared<QList<AgentResult>>();

    std::function<void(int)> runNext = [this, tasks, results, callback, &runNext](int index) {
        if (index >= tasks.size()) {
            callback(*results);
            return;
        }

        executeTask(tasks[index], [results, index, tasks, &runNext](const AgentResult& result) {
            results->append(result);
            runNext(index + 1);
        });
    };

    runNext(0);
}

void AgentOrchestrator::setLLMProvider(LLMProvider* provider)
{
    m_llmProvider = provider;
    for (auto& agent : m_agents) {
        agent->setLLMProvider(provider);
    }
}

void AgentOrchestrator::setToolRegistry(ToolRegistry* registry)
{
    m_toolRegistry = registry;
    for (auto& agent : m_agents) {
        agent->setToolRegistry(registry);
    }
}

} // namespace neurx
