import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import NeurXCode

// ── ToolCallCard ──────────────────────────────────────────────────────────────
//  Collapsible card that shows a tool invocation and its result.

Item {
    id: root

    required property string toolName
    required property string toolStatus   // "pending" | "running" | "done" | "error"
    required property string toolArgs
    required property string toolResult
    property var toolCodeChange: ({})

    implicitHeight: header.height + (expanded ? body.implicitHeight + 8 : 0)

    Behavior on implicitHeight { NumberAnimation { duration: 150 } }

    property bool expanded: toolStatus === "error"
    property bool isPatchTool: toolName === "patch"
    property bool hasPatchPreview: isPatchTool && toolResult.indexOf("Patch is applicable.") === 0
    readonly property bool hasCodeChange: {
        const value = root.toolCodeChange || {}
        return Object.keys(value).length > 0
    }
    readonly property bool codeChangeApproved: hasCodeChange && !!(root.toolCodeChange.approved ? true : false)
    readonly property bool codeChangeValid: hasCodeChange && !!(root.toolCodeChange.validation ? root.toolCodeChange.validation.isValid : false)
    readonly property bool codeChangeReviewed: hasCodeChange && !!(root.toolCodeChange.review ? root.toolCodeChange.review.canMerge : false)
    readonly property string codeChangeSummary: hasCodeChange
        ? (root.toolCodeChange.summary
            || (root.toolCodeChange.review ? root.toolCodeChange.review.summary : "")
            || (root.toolCodeChange.validation ? root.toolCodeChange.validation.summary : "")
            || "")
        : ""

    // ── Header ────────────────────────────────────────────────────────────
    Rectangle {
        id: header
        width: parent.width
        height: 34
        radius: Theme.radius
        color: Theme.surface
        border.color: statusColor

        readonly property color statusColor: {
            switch (root.toolStatus) {
            case "running":  return Theme.warning
            case "done":     return Theme.success
            case "error":    return Theme.error
            default:         return Theme.border
            }
        }

        RowLayout {
            anchors { fill: parent; leftMargin: 10; rightMargin: 10 }
            spacing: 8

            // Spinner or status icon
            Label {
                text: root.toolStatus === "running" ? "⟳"
                    : root.toolStatus === "done"    ? "✓"
                    : root.toolStatus === "error"   ? "✕"
                    : "○"
                color: header.statusColor
                font.pixelSize: Theme.fontMd

                RotationAnimator on rotation {
                    running: root.toolStatus === "running"
                    loops: Animation.Infinite
                    from: 0; to: 360
                    duration: 1000
                }
            }

            Label {
                text: root.toolName
                color: Theme.textPrimary
                font: Theme.monoFont
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Label {
                text: root.expanded ? "▲" : "▼"
                color: Theme.textMuted
                font.pixelSize: Theme.fontXs
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.expanded = !root.expanded
        }
    }

    // ── Body ──────────────────────────────────────────────────────────────
    Rectangle {
        id: body
        visible: root.expanded
        anchors { top: header.bottom; topMargin: 2; left: parent.left; right: parent.right }
        implicitHeight: bodyContent.implicitHeight + 16
        color: Theme.surfaceAlt
        radius: Theme.radius
        clip: true

            ColumnLayout {
                id: bodyContent
                anchors { fill: parent; margins: 10 }
                spacing: 8

            Rectangle {
                Layout.fillWidth: true
                visible: root.hasCodeChange
                radius: Theme.radius
                color: Theme.bg
                border.color: root.codeChangeApproved ? Theme.success
                              : root.codeChangeReviewed ? Theme.warning
                              : Theme.border
                implicitHeight: codeChangeColumn.implicitHeight + 12

                ColumnLayout {
                    id: codeChangeColumn
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: "Code Change"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSm
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: root.codeChangeApproved ? "approved"
                                : root.codeChangeReviewed ? "reviewed"
                                : root.codeChangeValid ? "validated"
                                : "tracked"
                            color: root.codeChangeApproved ? Theme.success
                                : root.codeChangeReviewed ? Theme.warning
                                : Theme.textMuted
                            font.pixelSize: Theme.fontXs
                            font.bold: true
                        }
                    }

                    Row {
                        spacing: 6
                        visible: root.toolCodeChange.changeSet !== undefined

                        Rectangle {
                            radius: 8
                            color: Theme.surfaceAlt
                            border.color: Theme.border
                            height: 20
                            width: changeSetLabel.implicitWidth + 12
                            Label {
                                id: changeSetLabel
                                anchors.centerIn: parent
                                text: root.toolCodeChange.changeSet && root.toolCodeChange.changeSet.totalFiles !== undefined
                                    ? root.toolCodeChange.changeSet.totalFiles + " files"
                                    : "change set"
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontXs
                            }
                        }

                        Rectangle {
                            radius: 8
                            color: Theme.surfaceAlt
                            border.color: Theme.border
                            height: 20
                            width: validationLabel.implicitWidth + 12
                            Label {
                                id: validationLabel
                                anchors.centerIn: parent
                                text: root.toolCodeChange.validation && root.toolCodeChange.validation.summary
                                    ? root.toolCodeChange.validation.summary
                                    : "validation"
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontXs
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Label {
                        visible: root.codeChangeSummary.length > 0
                        Layout.fillWidth: true
                        text: root.codeChangeSummary
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontXs
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // Args
            Label {
                text: "Arguments"
                color: Theme.textMuted
                font.pixelSize: Theme.fontSm
                visible: root.toolArgs.length > 0
            }
            TextEdit {
                Layout.fillWidth: true
                text: root.toolArgs
                readOnly: true; selectByMouse: true
                color: Theme.textPrimary
                font: Theme.monoFont
                wrapMode: TextEdit.Wrap
                visible: root.toolArgs.length > 0
            }

            // Result
            Rectangle {
                Layout.fillWidth: true
                height: 1; color: Theme.border
                visible: root.toolResult.length > 0
            }
            Label {
                text: root.hasPatchPreview ? "Diff Preview" : "Result"
                color: Theme.textMuted
                font.pixelSize: Theme.fontSm
                visible: root.toolResult.length > 0
            }
            TextEdit {
                Layout.fillWidth: true
                text: root.toolResult
                readOnly: true; selectByMouse: true
                color: root.toolStatus === "error" ? Theme.error : Theme.textPrimary
                font: Theme.monoFont
                wrapMode: TextEdit.Wrap
                visible: root.toolResult.length > 0
                textFormat: TextEdit.PlainText
            }
        }
    }
}
