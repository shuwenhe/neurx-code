#pragma once

#include "agent/AgentToolRegistry.h"
#include "agent/WorkspaceAnalyzer.h"
#include <memory>

/**
 * @class CodePerceptionTool
 * @brief Exposes the deep analysis capabilities of WorkspaceAnalyzer to the LLM.
 *
 * Capabilities:
 * - get_architecture: Summarizes modules and architectural patterns.
 * - analyze_dependencies: Shows what files depend on a specific file.
 * - get_impact_report: Predicts risks of changing a specific component.
 * - check_quality: Returns metrics like complexity and comment ratio.
 */
class CodePerceptionTool : public BaseTool {
    Q_OBJECT
public:
    explicit CodePerceptionTool(const QString& workspaceRoot, QObject* parent = nullptr);

    QString name()        const override { return "code_perception"; }
    QString description() const override {
        return "Perform deep architectural and dependency analysis on the workspace. "
               "Use this to understand project structure, find circular dependencies, "
               "or assess the impact of a planned change.";
    }

    QJsonObject parametersSchema() const override;
    ToolResult  execute(const QString &callId, const QJsonObject &args) override;
    QString     summary(const QJsonObject &args) const override;

private:
    QString m_workspaceRoot;
    std::unique_ptr<WorkspaceAnalyzer> m_analyzer;
};
