#pragma once
#include "agent/AgentToolRegistry.h"
#include <QString>
#include <QJsonObject>

/**
 * @class RequestPermissionTool
 * @brief Tool for the agent to explicitly request additional permissions
 */
class RequestPermissionTool : public BaseTool {
    Q_OBJECT
public:
    explicit RequestPermissionTool(QObject *parent = nullptr);

    QString name() const override { return "request_permission"; }
    QString description() const override {
        return "Request additional permissions from the user. "
               "Use this if you encounter a permission denied error or "
               "if you know you will need sensitive access for a task.";
    }

    QJsonObject parametersSchema() const override;
    ToolResult execute(const QString &callId, const QJsonObject &args) override;
    QString summary(const QJsonObject &args) const override;
};

