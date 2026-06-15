#pragma once

#include "agent/AgentToolRegistry.h"
#include "agent/ErrorRecoveryManager.h"

/**
 * @class RecoveryTool
 * @brief Tool for the agent to roll back the workspace to a previous checkpoint.
 */
class RecoveryTool : public BaseTool {
    Q_OBJECT
public:
    explicit RecoveryTool(ErrorRecoveryManager* manager, QObject *parent = nullptr)
        : BaseTool(parent), m_manager(manager) {}

    QString name() const override { return "rollback_to_checkpoint"; }
    QString description() const override {
        return "Roll back the workspace to a previous state captured in a checkpoint. "
               "Use this if you made a mistake or want to undo several recent changes.";
    }

    QJsonObject parametersSchema() const override {
        QJsonObject schema;
        schema["type"] = "object";
        QJsonObject props;

        QJsonObject checkpointId;
        checkpointId["type"] = "string";
        checkpointId["description"] = "The ID of the checkpoint to roll back to.";
        props["checkpointId"] = checkpointId;

        schema["properties"] = props;
        schema["required"] = QJsonArray() << "checkpointId";
        return schema;
    }

    ToolResult execute(const QString &callId, const QJsonObject &args) override {
        QString checkpointId = args.value("checkpointId").toString();
        if (checkpointId.isEmpty()) {
            return {callId, name(), true, "Missing checkpointId parameter."};
        }

        if (!m_manager) {
            return {callId, name(), true, "ErrorRecoveryManager not available."};
        }

        bool success = m_manager->rollback(checkpointId);
        if (success) {
            return {callId, name(), false, QString("Successfully rolled back to checkpoint %1.").arg(checkpointId)};
        } else {
            return {callId, name(), true, QString("Failed to roll back to checkpoint %1. It may not exist.").arg(checkpointId)};
        }
    }

private:
    ErrorRecoveryManager* m_manager;
};

/**
 * @class ListCheckpointsTool
 * @brief Tool for the agent to list available recovery checkpoints.
 */
class ListCheckpointsTool : public BaseTool {
    Q_OBJECT
public:
    explicit ListCheckpointsTool(ErrorRecoveryManager* manager, QObject *parent = nullptr)
        : BaseTool(parent), m_manager(manager) {}

    QString name() const override { return "list_checkpoints"; }
    QString description() const override {
        return "List available recovery checkpoints that can be used to undo changes.";
    }

    QJsonObject parametersSchema() const override {
        return {{"type", "object"}, {"properties", QJsonObject()}};
    }

    ToolResult execute(const QString &callId, const QJsonObject &args) override {
        Q_UNUSED(args);
        if (!m_manager) {
            return {callId, name(), true, "ErrorRecoveryManager not available."};
        }

        auto checkpoints = m_manager->getAllCheckpoints();
        if (checkpoints.isEmpty()) {
            return {callId, name(), false, "No checkpoints available."};
        }

        QStringList lines;
        lines << "Available Checkpoints:";
        for (const auto &cp : checkpoints) {
            lines << QString("- %1: %2 (%3)")
                    .arg(cp.id)
                    .arg(cp.description.isEmpty() ? "No description" : cp.description)
                    .arg(cp.createdAt.toString(Qt::ISODate));
        }

        return {callId, name(), false, lines.join("\n")};
    }

private:
    ErrorRecoveryManager* m_manager;
};
