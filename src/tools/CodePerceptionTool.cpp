#include "tools/CodePerceptionTool.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

CodePerceptionTool::CodePerceptionTool(const QString& workspaceRoot, QObject* parent)
    : BaseTool(parent), m_workspaceRoot(workspaceRoot)
{
    m_analyzer = std::make_unique<WorkspaceAnalyzer>(this);
    m_analyzer->analyzeWorkspace(m_workspaceRoot);
}

QJsonObject CodePerceptionTool::parametersSchema() const
{
    return QJsonDocument::fromJson(R"({
        "type": "object",
        "properties": {
            "action": {
                "type": "string",
                "enum": ["get_architecture", "analyze_dependencies", "get_impact_report", "check_quality"],
                "description": "The type of perception analysis to perform."
            },
            "file_path": {
                "type": "string",
                "description": "The specific file to analyze (required for get_impact_report)."
            }
        },
        "required": ["action"]
    })").object();
}

QString CodePerceptionTool::summary(const QJsonObject &args) const
{
    QString action = args["action"].toString();
    if (action == "get_impact_report") {
        return QString("Perceiving impact of change to: %1").arg(args["file_path"].toString());
    }
    return QString("Performing project perception: %1").arg(action);
}

ToolResult CodePerceptionTool::execute(const QString &callId, const QJsonObject &args)
{
    QString action = args["action"].toString();
    QJsonObject result;

    if (action == "get_architecture") {
        result["insights"] = m_analyzer->getArchitectureInsights();
        result["modules"] = QJsonArray::fromStringList(m_analyzer->identifyModules());
        result["layer_analysis"] = m_analyzer->getLayerAnalysis();
    }
    else if (action == "analyze_dependencies") {
        auto deps = m_analyzer->analyzeDependencies();
        result["circular_dependencies"] = deps.circulardependencyCount;
        result["dependency_score"] = (double)deps.dependencyScore;
        result["direct_dependencies"] = QJsonArray::fromStringList(deps.directDependencies);
    }
    else if (action == "get_impact_report") {
        QString filePath = args["file_path"].toString();
        if (filePath.isEmpty()) {
            return {callId, name(), true, "file_path is required for get_impact_report."};
        }
        QStringList dependents = m_analyzer->getFileDependents(filePath);
        result["file"] = filePath;
        result["direct_dependents"] = QJsonArray::fromStringList(dependents);
        result["impact_warning"] = dependents.isEmpty() ? "Low risk" : QString("High risk: %1 files depend on this.").arg(dependents.size());
    }
    else if (action == "check_quality") {
        auto report = m_analyzer->generateQualityReport();
        result["overall_score"] = (double)report.overallScore;
        result["issues_count"] = report.issues;
        result["warnings_count"] = report.warnings;
        result["violated_rules"] = QJsonArray::fromStringList(report.violatedRules);
    }
    else {
        return {callId, name(), true, "Unknown perception action."};
    }

    QString content = QJsonDocument(result).toJson(QJsonDocument::Indented);
    return {callId, name(), false, content};
}
