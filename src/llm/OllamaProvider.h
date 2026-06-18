#pragma once
#include "llm/LLMProvider.h"
#include <QNetworkAccessManager>
#include <QJsonObject>

// ── OllamaProvider ────────────────────────────────────────────────────────────
//  Connects to a local Ollama instance (default: http://localhost:11434).
//  Supports streaming via the /api/chat endpoint.
//  Model list is fetched dynamically from /api/tags.
//  Supports tool calling (function calling) for Ollama 0.3.0+

class OllamaProvider : public LLMProvider {
    Q_OBJECT
public:
    explicit OllamaProvider(QObject *parent = nullptr);

    QString     providerId()      const override { return "ollama"; }
    QString     displayName()     const override { return "Ollama (Local)"; }
    QStringList availableModels() const override { return m_cachedModels; }

    void sendRequest(const LLMRequest &request) override;
    void cancel() override;

    // Refresh model list from the Ollama daemon.
    void refreshModels();

signals:
    void modelsRefreshed(const QStringList &models);

private:
    struct PendingToolCall {
        QString id;
        QString name;
        QString args;
    };

    QNetworkAccessManager *m_nam{nullptr};
    QNetworkReply         *m_reply{nullptr};
    QStringList            m_cachedModels;
    QByteArray             m_buffer;
    QString                m_streamText;
    QMap<int, PendingToolCall> m_pendingToolCalls;
    LLMResponse            m_partialResponse;

    void handleStreamChunk(const QByteArray &chunk);
    void parseDelta(const QJsonObject &delta);
    QJsonObject parseToolArguments(const QString &raw, const QString &callId) const;
};
