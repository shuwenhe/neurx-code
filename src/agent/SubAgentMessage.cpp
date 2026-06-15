#include "SubAgentMessage.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>

// ──────────────────────────────────────────────────────────────────────────────
// SubAgentMessage Implementation
// ──────────────────────────────────────────────────────────────────────────────

SubAgentMessage::SubAgentMessage()
    : messageId(QUuid::createUuid().toString()), sentAt(QDateTime::currentDateTime())
{
}

SubAgentMessage::SubAgentMessage(const QString& agentId, SubAgentMessageType type)
    : messageId(QUuid::createUuid().toString()), agentId(agentId), type(type),
      sentAt(QDateTime::currentDateTime())
{
}

QString SubAgentMessage::toJsonString() const
{
    QJsonObject obj;
    obj["messageId"] = messageId;
    obj["agentId"] = agentId;
    obj["type"] = static_cast<int>(type);
    obj["sentAt"] = sentAt.toString(Qt::ISODate);
    obj["receivedAt"] = receivedAt.toString(Qt::ISODate);
    obj["priority"] = priority;
    obj["payload"] = payload;
    obj["metadata"] = metadata;
    
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

SubAgentMessage SubAgentMessage::fromJsonString(const QString& json)
{
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    QJsonObject obj = doc.object();
    
    SubAgentMessage msg;
    msg.messageId = obj["messageId"].toString();
    msg.agentId = obj["agentId"].toString();
    msg.type = static_cast<SubAgentMessageType>(obj["type"].toInt());
    msg.sentAt = QDateTime::fromString(obj["sentAt"].toString(), Qt::ISODate);
    msg.receivedAt = QDateTime::fromString(obj["receivedAt"].toString(), Qt::ISODate);
    msg.priority = obj["priority"].toInt();
    msg.payload = obj["payload"].toObject();
    msg.metadata = obj["metadata"].toObject();
    
    return msg;
}

// ──────────────────────────────────────────────────────────────────────────────
// SubAgentTask Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString SubAgentTask::toJson() const
{
    QJsonObject obj;
    obj["taskId"] = taskId;
    obj["type"] = type;
    obj["description"] = description;
    obj["parameters"] = parameters;
    
    QJsonArray depsArray;
    for (const auto& dep : dependencies) {
        depsArray.append(dep);
    }
    obj["dependencies"] = depsArray;
    
    obj["priority"] = priority;
    obj["timeoutMs"] = timeoutMs;
    obj["createdAt"] = createdAt.toString(Qt::ISODate);
    obj["scheduledAt"] = scheduledAt.toString(Qt::ISODate);
    
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

SubAgentTask SubAgentTask::fromJson(const QJsonObject& obj)
{
    SubAgentTask task;
    task.taskId = obj["taskId"].toString();
    task.type = obj["type"].toString();
    task.description = obj["description"].toString();
    task.parameters = obj["parameters"].toObject();
    task.priority = obj["priority"].toInt();
    task.timeoutMs = obj["timeoutMs"].toInt();
    task.createdAt = QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
    task.scheduledAt = QDateTime::fromString(obj["scheduledAt"].toString(), Qt::ISODate);
    
    QJsonArray depsArray = obj["dependencies"].toArray();
    for (const auto& dep : depsArray) {
        task.dependencies.append(dep.toString());
    }
    
    return task;
}

// ──────────────────────────────────────────────────────────────────────────────
// SubAgentResult Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString SubAgentResult::toJson() const
{
    QJsonObject obj;
    obj["taskId"] = taskId;
    obj["agentId"] = agentId;
    obj["success"] = success;
    obj["errorMessage"] = errorMessage;
    obj["data"] = data;
    obj["intermediateResults"] = intermediateResults;
    obj["executionTimeMs"] = executionTimeMs;
    obj["tokensUsed"] = tokensUsed;
    obj["qualityScore"] = qualityScore;
    obj["completedAt"] = completedAt.toString(Qt::ISODate);
    
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

SubAgentResult SubAgentResult::fromJson(const QJsonObject& obj)
{
    SubAgentResult result;
    result.taskId = obj["taskId"].toString();
    result.agentId = obj["agentId"].toString();
    result.success = obj["success"].toBool();
    result.errorMessage = obj["errorMessage"].toString();
    result.data = obj["data"].toObject();
    result.intermediateResults = obj["intermediateResults"].toArray();
    result.executionTimeMs = obj["executionTimeMs"].toInt();
    result.tokensUsed = obj["tokensUsed"].toInt();
    result.qualityScore = obj["qualityScore"].toDouble();
    result.completedAt = QDateTime::fromString(obj["completedAt"].toString(), Qt::ISODate);
    
    return result;
}

// ──────────────────────────────────────────────────────────────────────────────
// SubAgentProgress Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString SubAgentProgress::toJson() const
{
    QJsonObject obj;
    obj["taskId"] = taskId;
    obj["percentComplete"] = percentComplete;
    obj["currentStage"] = currentStage;
    obj["statusMessage"] = statusMessage;
    obj["estimatedRemainingMs"] = estimatedRemainingMs;
    
    QJsonArray completed;
    for (const auto& step : completedSteps) {
        completed.append(step);
    }
    obj["completedSteps"] = completed;
    
    QJsonArray remaining;
    for (const auto& step : remainingSteps) {
        remaining.append(step);
    }
    obj["remainingSteps"] = remaining;
    
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

// ──────────────────────────────────────────────────────────────────────────────
// SubAgentHealthStatus Implementation
// ──────────────────────────────────────────────────────────────────────────────

QString SubAgentHealthStatus::toJson() const
{
    QJsonObject obj;
    obj["agentId"] = agentId;
    obj["isAlive"] = isAlive;
    obj["isIdle"] = isIdle;
    obj["runningTaskCount"] = runningTaskCount;
    obj["totalTasksProcessed"] = totalTasksProcessed;
    obj["cpuUsagePercent"] = cpuUsagePercent;
    obj["memoryUsageMb"] = memoryUsageMb;
    obj["lastErrorMessage"] = lastErrorMessage;
    obj["lastActivityAt"] = lastActivityAt.toString(Qt::ISODate);
    
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

// ──────────────────────────────────────────────────────────────────────────────
// ConsensusResult Implementation
// ──────────────────────────────────────────────────────────────────────────────

QVector<SubAgentVote> ConsensusResult::getVotesFor(const QString& decision) const
{
    QVector<SubAgentVote> result;
    for (const auto& vote : votes) {
        if (vote.decision == decision) {
            result.append(vote);
        }
    }
    return result;
}

int ConsensusResult::getVoteCount(const QString& decision) const
{
    return getVotesFor(decision).size();
}
