import QtQuick 6.2
import QtQuick.Controls 6.2
import QtQuick.Layouts 6.2
import NeurXCode

Item {
    id: root

    required property var executionTimeline
    property string currentThreadId: ""
    property string timelineFilter: "all"

    readonly property bool hasTimeline: executionTimeline && executionTimeline.length > 0
    readonly property var filteredTimeline: {
        const items = root.executionTimeline || []
        if (root.timelineFilter === "all")
            return items
        return items.filter(function(item) {
            return timelineGroup(item.kind) === root.timelineFilter
        })
    }

    readonly property var displayedTimeline: collapseCodeChangeTimeline(root.filteredTimeline.slice(-20))

    function timelineGroup(kind) {
        switch (kind) {
        case "assistant_message":
        case "user_message":
        case "system_message":
            return "messages";
        case "approval":
            return "approvals";
        case "code_change_chain":
        case "code_change_track":
        case "code_change_validate":
        case "code_change_review":
        case "code_change_approval":
            return "files";
        case "tool_output":
            return "output";
        case "command_execution":
            return "commands";
        case "file_change":
            return "files";
        case "search":
        case "memory":
        case "knowledge":
            return "search";
        case "web":
            return "web";
        case "subagent_tool":
            return "subagent";
        default:
            return "tools";
        }
    }

    function timelineIcon(kind, status) {
        if (status === "error")
            return "!";
        if (status === "running")
            return "▶";
        switch (kind) {
        case "assistant_message":
        case "user_message":
        case "system_message":
            return "✉";
        case "command_execution":
            return "⌘";
        case "file_change":
            return "✎";
        case "approval":
            return "✓";
        case "code_change_chain":
            return "⟡";
        case "code_change_track":
            return "⊕";
        case "code_change_validate":
            return "✔";
        case "code_change_review":
            return "☑";
        case "code_change_approval":
            return "★";
        case "tool_output":
            return "≋";
        case "search":
        case "memory":
        case "knowledge":
            return "⌕";
        case "web":
            return "🌐";
        case "subagent_tool":
            return "🤖";
        case "tool":
            return "⋯";
        default:
            return "•";
        }
    }

    function timelineStatusColor(status) {
        if (status === "error")
            return Theme.error;
        if (status === "running")
            return Theme.accent;
        return Theme.textMuted;
    }

    function timelineKindLabel(kind) {
        switch (kind) {
        case "assistant_message":
            return "Assistant";
        case "user_message":
            return "User";
        case "system_message":
            return "System";
        case "approval":
            return "Approval";
        case "code_change_chain":
            return "Code Change";
        case "code_change_track":
            return "Track";
        case "code_change_validate":
            return "Validate";
        case "code_change_review":
            return "Review";
        case "code_change_approval":
            return "Approve";
        case "tool_output":
            return "Output";
        case "command_execution":
            return "Command";
        case "file_change":
            return "File";
        case "search":
            return "Search";
        case "memory":
            return "Memory";
        case "knowledge":
            return "Knowledge";
        case "web":
            return "Web";
        case "subagent_tool":
            return "Sub-Agent";
        default:
            return kind || "Event";
        }
    }

    function codeChangeStepLabel(kind) {
        switch (kind) {
        case "code_change_track":
            return "Track";
        case "code_change_validate":
            return "Validate";
        case "code_change_review":
            return "Review";
        case "code_change_approval":
            return "Approve";
        case "tool_execution":
            return "Apply";
        default:
            return timelineKindLabel(kind);
        }
    }

    function codeChangeStepIcon(kind, status) {
        if (status === "error")
            return "✕";
        if (status === "running")
            return "●";
        switch (kind) {
        case "code_change_track":
            return "⊕";
        case "code_change_validate":
            return "✔";
        case "code_change_review":
            return "☑";
        case "code_change_approval":
            return "★";
        case "tool_execution":
            return "↺";
        default:
            return "•";
        }
    }

    function codeChangeChainText(steps) {
        if (!steps || steps.length === 0)
            return "Track → Validate → Review → Approve → Apply";
        return steps.map(function(step) {
            return step.label || timelineKindLabel(step.kind);
        }).join(" → ");
    }

    function codeChangeStepVariant(step) {
        if (!step)
            return "idle"
        if (step.status === "error")
            return "error"
        if (step.status === "running")
            return "current"
        if (step.separatorAfter !== true)
            return "terminal"
        return "done"
    }

    function codeChangeStepEndIcon(step) {
        if (!step)
            return "•"
        if (step.status === "error")
            return "✕"
        if (step.status === "running")
            return "●"
        return "✓"
    }

    function isCodeChangeKind(kind) {
        return kind === "code_change_track"
            || kind === "code_change_validate"
            || kind === "code_change_review"
            || kind === "code_change_approval"
            || kind === "tool_execution";
    }

    function collapseCodeChangeTimeline(items) {
        const source = items || []
        const result = []
        let i = 0
        while (i < source.length) {
            const item = source[i]
            if (item && item.kind === "code_change_track") {
                const group = {
                    kind: "code_change_chain",
                    title: item.title || "Code change pipeline",
                    status: item.status || "done",
                    toolName: item.toolName || "",
                    callId: item.callId || "",
                    timestamp: item.timestamp || "",
                    details: item.details || "",
                    steps: []
                }
                let applyStep = null
                let j = i
                while (j < source.length) {
                    const candidate = source[j]
                    if (!candidate || candidate.callId !== group.callId || !isCodeChangeKind(candidate.kind))
                        break
                    if (candidate.kind === "tool_execution") {
                        applyStep = {
                            kind: candidate.kind,
                            label: codeChangeStepLabel(candidate.kind),
                            icon: codeChangeStepIcon(candidate.kind, candidate.status),
                            status: candidate.status || "done",
                            title: candidate.title || "",
                            details: candidate.details || "",
                            timestamp: candidate.timestamp || ""
                        }
                    } else {
                        group.steps.push({
                            kind: candidate.kind,
                            label: codeChangeStepLabel(candidate.kind),
                            icon: codeChangeStepIcon(candidate.kind, candidate.status),
                            status: candidate.status || "done",
                            title: candidate.title || "",
                            details: candidate.details || "",
                            timestamp: candidate.timestamp || ""
                        })
                    }
                    group.status = candidate.status || group.status
                    group.details = candidate.details || group.details
                    j += 1
                }
                if (applyStep !== null)
                    group.steps.push(applyStep)
                for (let k = 0; k < group.steps.length; ++k)
                    group.steps[k].separatorAfter = k < group.steps.length - 1
                result.push(group)
                i = j
                continue
            }

            result.push(item)
            i += 1
        }
        return result
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bg
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.surface
        border.color: Theme.border
        radius: Theme.radius + 2

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: headerColumn.implicitHeight + 18
                color: Theme.surfaceAlt
                radius: Theme.radius + 2
                border.color: Theme.border

                ColumnLayout {
                    id: headerColumn
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: "Execution Timeline"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontMd
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }

                        Label {
                            text: root.timelineFilter === "all"
                                ? root.executionTimeline.length + " events"
                                : root.filteredTimeline.length + " / " + root.executionTimeline.length + " events"
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontXs
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Active thread: " + (root.currentThreadId || "none")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontXs
                        elide: Text.ElideRight
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                Repeater {
                    model: [
                        { key: "all", label: "All" },
                        { key: "messages", label: "Messages" },
                        { key: "approvals", label: "Approvals" },
                        { key: "output", label: "Output" },
                        { key: "commands", label: "Commands" },
                        { key: "files", label: "Files" },
                        { key: "search", label: "Search" },
                        { key: "web", label: "Web" },
                        { key: "subagent", label: "Sub-Agents" },
                        { key: "tools", label: "Tools" }
                    ]

                    delegate: Button {
                        required property var modelData
                        text: modelData.label
                        checkable: true
                        checked: root.timelineFilter === modelData.key
                        implicitHeight: 26
                        onClicked: root.timelineFilter = modelData.key
                    }
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.hasTimeline
                color: Theme.surfaceAlt
                radius: Theme.radius
                border.color: Theme.border

                Flickable {
                    id: flickable
                    anchors.fill: parent
                    anchors.margins: 6
                    clip: true
                    contentWidth: width
                    contentHeight: timelineColumn.implicitHeight

                    ScrollBar.vertical: CustomScrollBar {
                        anchors.right: flickable.right
                        anchors.rightMargin: 0
                    }

                    ColumnLayout {
                        id: timelineColumn
                        width: parent.width
                        spacing: 6

                        Repeater {
                            model: root.displayedTimeline

                            delegate: Rectangle {
                                required property var modelData

                                Layout.fillWidth: true
                                radius: Theme.radius
                                color: Theme.surface
                                border.color: Theme.border
                                implicitHeight: eventRow.implicitHeight + 10

                                RowLayout {
                                    id: eventRow
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    spacing: 8

                                    Label {
                                        text: timelineIcon(modelData.kind, modelData.status)
                                        color: timelineStatusColor(modelData.status)
                                        font.pixelSize: Theme.fontSm
                                        font.bold: modelData.status === "error" || modelData.status === "running"
                                    }

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 2

                                        ColumnLayout {
                                            visible: modelData.kind === "code_change_chain"
                                            Layout.fillWidth: true
                                            spacing: 4

                                            Label {
                                                Layout.fillWidth: true
                                                text: modelData.title || "Code change pipeline"
                                                color: Theme.textPrimary
                                                font.pixelSize: Theme.fontXs
                                                elide: Text.ElideRight
                                            }

                                            Label {
                                                Layout.fillWidth: true
                                                text: codeChangeChainText(modelData.steps || [])
                                                color: Theme.textMuted
                                                font.pixelSize: Theme.fontXs
                                                elide: Text.ElideRight
                                            }

                                            RowLayout {
                                                Layout.fillWidth: true
                                                spacing: 0

                                                Repeater {
                                                    model: modelData.steps || []

                                                    delegate: RowLayout {
                                                        required property var modelData
                                                        spacing: 0

                                                        Rectangle {
                                                            property bool isCurrentStep: codeChangeStepVariant(modelData) === "current"
                                                            property bool isTerminalStep: codeChangeStepVariant(modelData) === "terminal"
                                                            property real pulse: 0.0
                                                            radius: 10
                                                            border.color: isCurrentStep || isTerminalStep ? Theme.accent : Theme.border
                                                            border.width: isCurrentStep || isTerminalStep ? 2 : 1
                                                            implicitHeight: 20
                                                            implicitWidth: stepContent.implicitWidth + (isTerminalStep ? 24 : 18)
                                                            color: codeChangeStepVariant(modelData)
                                                                === "error" ? Theme.error
                                                                : codeChangeStepVariant(modelData)
                                                                === "current" ? Theme.accent
                                                                : codeChangeStepVariant(modelData)
                                                                === "terminal" ? Theme.surface
                                                                : Theme.surfaceAlt
                                                            opacity: isCurrentStep || isTerminalStep ? 1.0 : 0.95
                                                            scale: isCurrentStep || isTerminalStep ? 1.02 : 1.0

                                                            SequentialAnimation on pulse {
                                                                running: isCurrentStep
                                                                loops: Animation.Infinite
                                                                NumberAnimation {
                                                                    to: 1.0
                                                                    duration: 900
                                                                    easing.type: Easing.InOutSine
                                                                }
                                                                NumberAnimation {
                                                                    to: 0.0
                                                                    duration: 900
                                                                    easing.type: Easing.InOutSine
                                                                }
                                                            }

                                                            Rectangle {
                                                                visible: parent.isCurrentStep || parent.isTerminalStep
                                                                anchors.left: parent.left
                                                                anchors.right: parent.right
                                                                anchors.top: parent.top
                                                                height: 2
                                                                radius: 10
                                                                color: parent.isTerminalStep ? Theme.accent : "white"
                                                                opacity: parent.isTerminalStep ? 0.45 : 0.25
                                                            }

                                                            Rectangle {
                                                                visible: parent.isCurrentStep || parent.isTerminalStep
                                                                anchors.fill: parent
                                                                radius: parent.radius
                                                                color: "transparent"
                                                                border.color: Theme.accent
                                                                border.width: 1
                                                                opacity: parent.isTerminalStep ? 0.22 : 0.12 + (parent.pulse * 0.16)
                                                            }

                                                            Row {
                                                                id: stepContent
                                                                anchors.centerIn: parent
                                                                spacing: 4

                                                                Label {
                                                                    text: modelData.icon || "•"
                                                                    color: codeChangeStepVariant(modelData) === "current"
                                                                        || codeChangeStepVariant(modelData) === "terminal"
                                                                        || codeChangeStepVariant(modelData) === "error" ? "white"
                                                                        : Theme.textPrimary
                                                                    font.pixelSize: Theme.fontXs
                                                                    font.bold: true
                                                                }

                                                                Label {
                                                                    id: stepLabel
                                                                    text: modelData.label || ""
                                                                    color: codeChangeStepVariant(modelData) === "current"
                                                                        || codeChangeStepVariant(modelData) === "terminal"
                                                                        || codeChangeStepVariant(modelData) === "error" ? "white"
                                                                        : Theme.textPrimary
                                                                    font.pixelSize: Theme.fontXs
                                                                    font.bold: true
                                                                }
                                                            }
                                                        }

                                                        Rectangle {
                                                            visible: modelData.separatorAfter === true
                                                            Layout.preferredWidth: 12
                                                            Layout.preferredHeight: 1
                                                            color: Theme.border
                                                            opacity: 0.85
                                                        }

                                                    Rectangle {
                                                        visible: modelData.separatorAfter !== true
                                                        Layout.preferredWidth: 24
                                                        Layout.preferredHeight: 20
                                                        radius: 10
                                                        color: modelData.status === "error" ? Theme.error
                                                            : modelData.status === "running" ? Theme.warning
                                                            : Theme.surface
                                                        border.color: modelData.status === "error" ? Theme.error
                                                            : modelData.status === "running" ? Theme.accent
                                                            : Theme.accent
                                                        border.width: 1

                                                        Label {
                                                            anchors.centerIn: parent
                                                                text: codeChangeStepEndIcon(modelData)
                                                                color: modelData.status === "error" || modelData.status === "running"
                                                                    ? "white"
                                                                    : Theme.textPrimary
                                                                font.pixelSize: Theme.fontXs
                                                                font.bold: true
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        Label {
                                            visible: modelData.kind !== "code_change_chain"
                                            Layout.fillWidth: true
                                            text: modelData.title || timelineKindLabel(modelData.kind)
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontXs
                                            elide: Text.ElideRight
                                        }

                                        Label {
                                            visible: modelData.kind !== "code_change_chain"
                                            Layout.fillWidth: true
                                            text: timelineKindLabel(modelData.kind)
                                                + ((modelData.toolName || "").length > 0 ? " · " + modelData.toolName : "")
                                                + ((modelData.details || "").length > 0 ? " · " + modelData.details : "")
                                            color: Theme.textMuted
                                            font.pixelSize: Theme.fontXs
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Label {
                                        text: modelData.timestamp || ""
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.fontXs
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Label {
                visible: !root.hasTimeline
                Layout.fillWidth: true
                text: "No execution events yet. Send a message or run a tool to start the timeline."
                color: Theme.textMuted
                font.pixelSize: Theme.fontXs
                wrapMode: Text.Wrap
            }
        }
    }
}
