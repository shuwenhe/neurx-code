#include "llm/OllamaProvider.h"
#include "llm/ToolCallRepair.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>

static constexpr char kDefaultBase[] = "http://localhost:11434";

namespace {

QString previewToolNames(const QJsonArray &tools, int maxCount = 8)
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

} // namespace

OllamaProvider::OllamaProvider(QObject *parent)
    : LLMProvider(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_endpoint = kDefaultBase;
    refreshModels();
}

void OllamaProvider::refreshModels()
{
    QNetworkRequest req(QUrl(m_endpoint + "/api/tags"));
    auto *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        const auto obj = QJsonDocument::fromJson(reply->readAll()).object();
        m_cachedModels.clear();
        for (const auto &m : obj["models"].toArray())
            m_cachedModels << m.toObject()["name"].toString();
        emit modelsRefreshed(m_cachedModels);
    });
}

void OllamaProvider::sendRequest(const LLMRequest &request)
{
    if (m_reply) cancel();
    
    // Reset streaming state
    m_buffer.clear();
    m_streamText.clear();
    m_pendingToolCalls.clear();
    m_partialResponse = LLMResponse();

    QJsonObject body;
    body["model"]  = request.model.isEmpty() ? (m_cachedModels.isEmpty() ? "llama3" : m_cachedModels.first())
                                              : request.model;
    body["stream"] = request.stream;

    // Build messages array
    QJsonArray messages;
    for (const auto &msg : request.messages) {
        QJsonObject m;
        switch (msg.role) {
        case MessageRole::System:    m["role"] = "system";    break;
        case MessageRole::User:      m["role"] = "user";      break;
        case MessageRole::Assistant: m["role"] = "assistant"; break;
        case MessageRole::Tool:      m["role"] = "tool";      break;
        }
        
        // Handle tool results
        if (msg.hasToolResults()) {
            for (const auto &tr : msg.toolResults) {
                QJsonObject tm;
                tm["role"] = "tool";
                tm["content"] = tr.content;
                messages.append(tm);
            }
            continue;
        }
        
        // Handle tool calls
        if (msg.hasToolCalls()) {
            QJsonArray calls;
            for (const auto &tc : msg.toolCalls) {
                QJsonObject call;
                call["id"] = tc.id;
                call["type"] = "function";
                QJsonObject fn;
                fn["name"] = tc.name;
                fn["arguments"] = QString::fromUtf8(QJsonDocument(tc.arguments).toJson(QJsonDocument::Compact));
                call["function"] = fn;
                calls.append(call);
            }
            m["tool_calls"] = calls;
        }
        
        m["content"] = msg.content;
        messages.append(m);
    }
    body["messages"] = messages;
    
    // Add tools if provided (Ollama 0.3.0+ supports OpenAI-compatible tool calling)
    if (!request.tools.isEmpty()) {
        body["tools"] = request.tools;
    }

    qInfo().noquote() << "[OllamaProvider] sendRequest:"
                      << "model=" << body.value(QStringLiteral("model")).toString()
                      << "messages=" << messages.size()
                      << "tools=" << request.tools.size()
                      << "toolNames=[" << previewToolNames(request.tools) << "]";

    QNetworkRequest req(QUrl(m_endpoint + "/api/chat"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_reply = m_nam->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        handleStreamChunk(m_reply->readAll());
    });
    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        if (m_reply->error() != QNetworkReply::NoError)
            emit requestError(m_reply->errorString());
        m_reply->deleteLater();
        m_reply = nullptr;
    });
}

void OllamaProvider::cancel()
{
    if (m_reply) { 
        m_reply->abort(); 
        m_reply->deleteLater(); 
        m_reply = nullptr; 
    }
    m_buffer.clear();
    m_streamText.clear();
    m_pendingToolCalls.clear();
}

void OllamaProvider::handleStreamChunk(const QByteArray &chunk)
{
    // Ollama streams NDJSON: one JSON object per line.
    m_buffer += chunk;

    while (true) {
        int nl = m_buffer.indexOf('\n');
        if (nl < 0) break;
        const QByteArray line = m_buffer.left(nl).trimmed();
        m_buffer = m_buffer.mid(nl + 1);
        if (line.isEmpty()) continue;

        const QJsonObject obj = QJsonDocument::fromJson(line).object();
        
        // Parse message delta
        const QJsonObject message = obj["message"].toObject();
        parseDelta(message);
        
        // Check if done
        if (obj["done"].toBool()) {
            // Finalize any pending tool calls
            for (auto it = m_pendingToolCalls.cbegin(); it != m_pendingToolCalls.cend(); ++it) {
                const auto &pending = it.value();
                if (pending.id.isEmpty() || pending.name.isEmpty())
                    continue;

                ToolCall tc;
                tc.id = pending.id;
                tc.name = pending.name;
                tc.arguments = parseToolArguments(pending.args, pending.id);
                m_partialResponse.message.toolCalls.append(tc);
            }

            if (!m_partialResponse.message.toolCalls.isEmpty()) {
                QStringList toolNames;
                for (const ToolCall &call : m_partialResponse.message.toolCalls)
                    toolNames << QStringLiteral("%1(%2)").arg(call.name, call.id);
                qInfo().noquote() << "[OllamaProvider] response tool_calls:"
                                  << toolNames.join(QStringLiteral(", "));
            } else {
                qInfo().noquote() << "[OllamaProvider] response tool_calls: none";
            }
            
            m_partialResponse.message.role = MessageRole::Assistant;
            m_partialResponse.message.content = m_streamText;
            
            TokenEvent te; 
            te.type = TokenEvent::Type::MessageEnd;
            emit tokenReceived(te);
            emit responseComplete(m_partialResponse);
        }
    }
}

void OllamaProvider::parseDelta(const QJsonObject &delta)
{
    // Handle text content
    if (delta.contains("content") && !delta["content"].isNull()) {
        const QString text = delta["content"].toString();
        if (!text.isEmpty()) {
            m_streamText += text;
            TokenEvent te; 
            te.type = TokenEvent::Type::TextDelta; 
            te.delta = text;
            emit tokenReceived(te);
        }
    }

    // Handle tool calls (Ollama 0.3.0+ uses OpenAI-compatible format)
    if (delta.contains("tool_calls")) {
        for (const auto &tcVal : delta["tool_calls"].toArray()) {
            const auto tc = tcVal.toObject();
            const int idx = tc["index"].toInt();
            const auto fn = tc["function"].toObject();
            auto &pending = m_pendingToolCalls[idx];
            
            if (tc.contains("id")) 
                pending.id = tc["id"].toString();
            if (fn.contains("name")) 
                pending.name = fn["name"].toString();
            if (fn.contains("arguments")) 
                pending.args += fn["arguments"].toString();

            qInfo().noquote() << "[OllamaProvider] tool_call delta:"
                              << "index=" << idx
                              << "id=" << pending.id
                              << "name=" << pending.name
                              << "argsChars=" << pending.args.size();
        }
    }
}

QJsonObject OllamaProvider::parseToolArguments(const QString &raw, const QString &callId) const
{
    bool ok = false;
    const QJsonObject obj = ToolCallRepair::repairJsonObject(raw, &ok);
    if (!ok) {
        qWarning() << "[OllamaProvider] Failed to parse tool arguments for callId" 
                   << callId << ":" << raw;
        return QJsonObject();
    }
    return obj;
}
