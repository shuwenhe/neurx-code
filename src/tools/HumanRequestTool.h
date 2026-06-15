#pragma once
#include "agent/AgentToolRegistry.h"
#include <QString>
#include <QJsonObject>

/**
 * @class HumanRequestTool
 * @brief Tool for asking the user for input or confirmation during agent execution
 */
class HumanRequestTool : public BaseTool {
    Q_OBJECT
public:
    explicit HumanRequestTool(QObject *parent = nullptr);

    QString name() const override { return "ask_user"; }
    QString description() const override {
        return "Ask the user a question or for clarification. Use this when you are stuck, "
               "need missing information, or need the user to perform an action outside the agent's capabilities.";
    }

    QJsonObject parametersSchema() const override;
    ToolResult execute(const QString &callId, const QJsonObject &args) override;
    QString summary(const QJsonObject &args) const override;
};

