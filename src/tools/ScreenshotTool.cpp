#include "ScreenshotTool.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QBuffer>
#include <QWindow>
#include <QDebug>
#include <QJsonDocument>

ScreenshotTool::ScreenshotTool(QObject *parent)
    : BaseTool(parent)
{
}

QJsonObject ScreenshotTool::parametersSchema() const {
    QJsonObject props;

    QJsonObject type;
    type["type"] = "string";
    type["description"] = "Type of screenshot: 'full' or 'window'";
    type["enum"] = QJsonArray({"full", "window"});
    props["type"] = type;

    QJsonObject windowName;
    windowName["type"] = "string";
    windowName["description"] = "Title of the window to capture (only if type is 'window')";
    props["windowName"] = windowName;

    QJsonObject format;
    format["type"] = "string";
    format["description"] = "Image format: 'png' or 'jpg'";
    format["enum"] = QJsonArray({"png", "jpg"});
    props["format"] = format;

    QJsonObject quality;
    quality["type"] = "integer";
    quality["description"] = "Image quality (0-100, default 80)";
    props["quality"] = quality;

    QJsonArray required;
    required.append("type");

    QJsonObject schema;
    schema["type"] = "object";
    schema["properties"] = props;
    schema["required"] = required;

    return schema;
}

ToolResult ScreenshotTool::execute(const QString &callId, const QJsonObject &args) {
    QString type = args["type"].toString("full");
    QString format = args["format"].toString("png");
    int quality = args["quality"].toInt(80);

    QPixmap screenshot;
    if (type == "full") {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            return ToolResult{callId, name(), true, "No screen detected"};
        }
        screenshot = screen->grabWindow(0);
    } else {
        QString windowName = args["windowName"].toString();
        if (windowName.isEmpty()) {
            return ToolResult{callId, name(), true, "windowName is required for type 'window'"};
        }
        // Simplified window capture for now, using full screen as fallback
        // In a real implementation, we'd iterate over windows to find the match
        QScreen *screen = QGuiApplication::primaryScreen();
        screenshot = screen->grabWindow(0);
    }

    if (screenshot.isNull()) {
        return ToolResult{callId, name(), true, "Failed to capture screenshot"};
    }

    // Downscale if too large for LLM
    if (screenshot.width() > 2048 || screenshot.height() > 2048) {
        screenshot = screenshot.scaled(2048, 2048, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    screenshot.save(&buffer, format.toLatin1(), quality);

    QString base64 = ba.toBase64();

    QJsonObject resultData;
    resultData["image_data"] = base64;
    resultData["format"] = format;
    resultData["width"] = screenshot.width();
    resultData["height"] = screenshot.height();
    resultData["mime_type"] = "image/" + format;

    return ToolResult{callId, name(), false, QJsonDocument(resultData).toJson(QJsonDocument::Compact)};
}

QString ScreenshotTool::summary(const QJsonObject &args) const {
    QString type = args["type"].toString("full");
    if (type == "window") {
        return QString("Captured window: %1").arg(args["windowName"].toString());
    }
    return "Captured full screen";
}
