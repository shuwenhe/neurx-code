#include "HumanRequestTool.h"
#include <QJsonObject>
#include <QJsonArray>

HumanRequestTool::HumanRequestTool(QObject *parent)
    : BaseTool(parent)
{
}

QJsonObject HumanRequestTool::parametersSchema() const {
    QJsonObject props;

    QJsonObject question;
    question["type"] = "string";
    question["description"] = "The question or request to send to the user.";
    props["question"] = question;

    QJsonObject options;
    options["type"] = "array";
    options["description"] = "Optional list of predefined answers for the user to choose from.";
    QJsonObject items;
    items["type"] = "string";
    options["items"] = items;
    props["options"] = options;

    QJsonArray required;
    required.append("question");

    QJsonObject schema;
    schema["type"] = "object";
    schema["properties"] = props;
    schema["required"] = required;

    return schema;
}

ToolResult HumanRequestTool::execute(const QString &callId, const QJsonObject &args) {
    QString question = args["question"].toString();
    QJsonArray options = args["options"].toArray();

    QJsonObject resultData;
    resultData["question"] = question;
    if (!options.isEmpty()) {
        resultData["options"] = options;
    }
    resultData["status"] = "waiting_for_user";

    // In a real system, this would trigger a UI modal and block until the user responds
    // For now, we return the request details
    return ToolResult{callId, name(), false, QJsonDocument(resultData).toJson(QJsonDocument::Compact)};
}

QString HumanRequestTool::summary(const QJsonObject &args) const {
    return QString("Asked user: %1").arg(args["question"].toString());
}

