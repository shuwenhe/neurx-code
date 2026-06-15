#include "RequestPermissionTool.h"
#include <QJsonObject>
#include <QJsonArray>

RequestPermissionTool::RequestPermissionTool(QObject *parent)
    : BaseTool(parent)
{
}

QJsonObject RequestPermissionTool::parametersSchema() const {
    QJsonObject props;

    QJsonObject permission;
    permission["type"] = "string";
    permission["description"] = "The name of the permission being requested (e.g. 'network', 'disk_read', 'full_disk_access')";
    props["permission"] = permission;

    QJsonObject reason;
    reason["type"] = "string";
    reason["description"] = "The reason why the agent needs this permission.";
    props["reason"] = reason;

    QJsonArray required;
    required.append("permission");
    required.append("reason");

    QJsonObject schema;
    schema["type"] = "object";
    schema["properties"] = props;
    schema["required"] = required;

    return schema;
}

ToolResult RequestPermissionTool::execute(const QString &callId, const QJsonObject &args) {
    QString perm = args["permission"].toString();
    QString reason = args["reason"].toString();

    QJsonObject resultData;
    resultData["permission"] = perm;
    resultData["reason"] = reason;
    resultData["status"] = "awaiting_approval";

    // In a real system, this would interact with the ToolPermissionManager
    // and show a UI prompt to the user.
    return ToolResult{callId, name(), false, QJsonDocument(resultData).toJson(QJsonDocument::Compact)};
}

QString RequestPermissionTool::summary(const QJsonObject &args) const {
    return QString("Requested permission '%1' for: %2").arg(args["permission"].toString(), args["reason"].toString());
}

