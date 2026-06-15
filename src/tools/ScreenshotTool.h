#pragma once
#include "agent/AgentToolRegistry.h"
#include <QString>
#include <QJsonObject>

/**
 * @class ScreenshotTool
 * @brief Tool for taking screenshots of the screen or specific windows
 *
 * Supports:
 * - Full screen capture
 * - Specific window capture (by name/ID)
 * - Compression and resizing for LLM compatibility
 */
class ScreenshotTool : public BaseTool {
    Q_OBJECT
public:
    explicit ScreenshotTool(QObject *parent = nullptr);

    QString name() const override { return "screenshot"; }
    QString description() const override {
        return "Capture a screenshot of the current screen or a specific window. "
               "Returns the image data as a base64 encoded string. "
               "Use this when you need to see the UI or the result of a web automation task.";
    }

    QJsonObject parametersSchema() const override;
    ToolResult execute(const QString &callId, const QJsonObject &args) override;
    QString summary(const QJsonObject &args) const override;

private:
    QString captureFullScreen();
    QString captureWindow(const QString &windowTitle);
};

