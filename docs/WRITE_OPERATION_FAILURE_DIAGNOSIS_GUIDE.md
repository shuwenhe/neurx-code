# Complete Write Operation Flow & Failure Diagnosis Guide

**Last Updated:** 2026-06-12
**Scope:** neurx-code, hermes-agent
**Status:** DIAGNOSTIC ANALYSIS

---

## Executive Summary

This document traces the **complete execution path** of a WriteTool invocation from LLM generation through successful file creation or failure reporting. It identifies 13 critical failure points where write operations can fail silently or with errors.

---

## Part 1: Complete Write Operation Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ USER SUBMITS MESSAGE                                                         │
│ agent.submitUserMessage(text)                                               │
└────────────────────┬────────────────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ PHASE 1: AGENT THINKING (AgentEngine::runLoop → LLM Request)                │
│                                                                              │
│ 1. AgentEngine.runLoop() starts                                             │
│ 2. Builds LLMRequest with:                                                  │
│    - Message history                                                        │
│    - Workspace context                                                      │
│    - Available tools (from AgentToolRegistry)                               │
│ 3. LLM receives request and generates response                              │
│ 4. Response contains: ToolCall[] with:                                      │
│    - name: "Write" (or variant like "write_file", "codex_agent")           │
│    - id: unique call ID (e.g., "call-12345")                               │
│    - arguments: { "file_path": "...", "new_text": "..." }                  │
│                                                                              │
│ ⚠️  FAILURE POINT #1: Tool name mismatch                                    │
│     If LLM generates "write_file" but registry only has "Write"            │
└────────────────────┬────────────────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ PHASE 2: TOOL CALL EXTRACTION                                               │
│                                                                              │
│ Response added to message history                                           │
│ for (const auto &call : response.message.toolCalls)                        │
│                                                                              │
│ ⚠️  FAILURE POINT #2: toolCalls array is empty                             │
│     LLM didn't generate WriteTool call                                      │
└────────────────────┬────────────────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ PHASE 3: APPROVAL CHECK (AgentEngine::shouldRequireApproval)               │
│                                                                              │
│ For each WriteTool call:                                                   │
│                                                                              │
│ ┌─ Check 1: ApprovalManager Policy ──────────────────────────────────────┐ │
│ │  if (m_approvalManager) {                                             │ │
│ │      policy = m_approvalManager->getPolicyFor("Write", resource)     │ │
│ │      if (policy == AskForApproval::Never) → AUTO-APPROVE             │ │
│ │      if (policy == OnRequest/Granular/UnlessTrusted) → REQUIRE       │ │
│ │  }                                                                    │ │
│ └────────────────────────────────────────────────────────────────────────┘ │
│                                                                              │
│ ┌─ Check 2: ExecutionStrategy & Risk Assessment ─────────────────────────┐ │
│ │  if (m_strategyManager) {                                              │ │
│ │      strategy = getStrategyForTool("Write")                            │ │
│ │      risk = assessToolCallRisk(call, strategyManager)                 │ │
│ │      require = needsApproval(risk, strategy)                           │ │
│ │  }                                                                      │ │
│ └────────────────────────────────────────────────────────────────────────┘ │
│                                                                              │
│ ┌─ Check 3: autoApproveTools Configuration ──────────────────────────────┐ │
│ │  if (!m_config.autoApproveTools)                                       │ │
│ │      return true  // ← REQUIRES APPROVAL                              │ │
│ │                                                                        │ │
│ │  const QString risk = approvalRiskLevel(call)  // "high"              │ │
│ │  return (risk == "high" || risk == "critical")  // → true             │ │
│ └────────────────────────────────────────────────────────────────────────┘ │
│                                                                              │
│ RESULT:                                                                      │
│   requiresApproval = true  →  EMIT toolApprovalRequired signal             │
│   requiresApproval = false →  SKIP TO PHASE 4 (Execution)                  │
│                                                                              │
│ ⚠️  FAILURE POINT #3: autoApproveTools set to false, no approval signal    │
│     UI may not connect to toolApprovalRequired signal                       │
│ ⚠️  FAILURE POINT #4: ApprovalManager not initialized (nullptr)             │
│     Approval policy checks skipped                                          │
└────────────────────┬────────────────────────────────────────────────────────┘
                     │
        ┌────────────┴────────────┐
        │                         │
        ▼                         ▼
    (Approval)              (No Approval)
        │                         │
        │                         ▼
        │      ┌──────────────────────────────────────────────────┐
        │      │ PHASE 4A: IMMEDIATE EXECUTION (Auto-approve)    │
        │      │ Skip to: Executor::execute() at Phase 4B        │
        │      └──────────────────────────────────────────────────┘
        │                         │
        ▼                         │
┌───────────────────────────────┐│
│ PHASE 4A: SEND APPROVAL       ││
│ SIGNAL TO QML UI              ││
│                               ││
│ emit toolApprovalRequired(    ││
│    call,                      ││
│    approvalRiskLevel(call)    ││
│ )                             ││
│                               ││
│ ⚠️  FAILURE POINT #5:         ││
│  Signal not connected to QML  ││
│  AgentController::           ││
│    connect(engine,          ││
│      toolApprovalRequired,  ││
│      ...onToolApprovalRequired) ││
│                               ││
│ ⚠️  FAILURE POINT #6:         ││
│  ToolApprovalDialog not shown ││
│  to user (QML not rendering)  ││
│                               ││
│ Agent loop WAITS for:        ││
│   m_pendingApprovals[callId] ││
│   to be removed              ││
│                               ││
│ Status → WAITING             ││
└────────┬───────────────────────┘│
         │                        │
         │ USER CLICKS "APPROVE"  │
         │                        │
         ▼                        │
┌─────────────────────────────────┐
│ PHASE 4B: USER APPROVAL         │
│                                 │
│ ToolApprovalDialog.approved()  │
│   ↓                             │
│ agent.approveTool(callId, true)│
│   ↓                             │
│ AgentController.onApproveTool()│
│   ↓                             │
│ AgentEngine.approveTool()       │
│   {                             │
│   QMutexLocker locker(&mutex)  │
│   m_pendingApprovals.remove()   │ ← UNBLOCKS agent loop
│   // No execution here! Just    │
│   // removes from pending       │
│   }                             │
│                                 │
│ ⚠️  FAILURE POINT #7:           │
│  User never sees dialog        │
│  (approval signal not connected)│
│ ⚠️  FAILURE POINT #8:           │
│  User clicks approve but       │
│  approveTool() not called      │
│  (QML button not connected)    │
└────────┬───────────────────────┘
         │
         │ approveTool() removes callId
         │ Agent loop continues
         │
         ▼
         │
         └──────────────────────────┐
                                    │
                                    ▼
                    ┌─────────────────────────────────────────────┐
                    │ PHASE 5: EXECUTE TOOL CALL                  │
                    │                                             │
                    │ Agent loop resumes (unblocked)              │
                    │                                             │
                    │ for (const auto &call : toolCalls)          │
                    │ {                                           │
                    │   emit toolExecuting(call)                  │
                    │   result = m_executor.execute(call)         │
                    │   emit toolFinished(result)                 │
                    │ }                                           │
                    │                                             │
                    │ ⚠️  FAILURE POINT #9:                       │
                    │  toolExecuting signal not connected         │
                    │  to update UI (no loading state shown)     │
                    └────────────┬────────────────────────────────┘
                                 │
                                 ▼
                    ┌─────────────────────────────────────────────┐
                    │ PHASE 6: EXECUTOR::EXECUTE                  │
                    │                                             │
                    │ Executor::execute(call) {                   │
                    │   BaseTool *tool =                          │
                    │     m_registry->tool(call.name)             │
                    │                                             │
                    │   if (!tool) {                              │
                    │     return error: "Unknown tool"            │
                    │   }                                         │
                    │                                             │
                    │   return tool->execute(call.id,             │
                    │                        call.arguments)      │
                    │ }                                           │
                    │                                             │
                    │ ⚠️  FAILURE POINT #10:                      │
                    │  WriteTool not registered in registry       │
                    │  (returns "Unknown tool" error)             │
                    │ ⚠️  FAILURE POINT #11:                      │
                    │  Wrong tool name used by LLM                │
                    │  ("write_file" vs "Write")                  │
                    └────────────┬────────────────────────────────┘
                                 │
                                 ▼
                    ┌─────────────────────────────────────────────┐
                    │ PHASE 7: WRITETOOL::EXECUTE                 │
                    │                                             │
                    │ WriteTool::execute(callId, args) {          │
                    │                                             │
                    │ Step 1: Validate inputs                     │
                    │ - file_path: ""  → error                    │
                    │ - new_text: ""   → warning (empty file ok) │
                    │                                             │
                    │ ⚠️  FAILURE POINT #12A:                     │
                    │  file_path is empty                         │
                    │                                             │
                    │ Step 2: Resolve path (safePath)             │
                    │ - Resolve relative to workspace root        │
                    │ - Check for path traversal attacks          │
                    │ → absPath = "/workspace/output.txt"         │
                    │                                             │
                    │ ⚠️  FAILURE POINT #12B:                     │
                    │  Path traversal detected                    │
                    │  absPath becomes empty ("")                 │
                    │                                             │
                    │ Step 3: Check sandbox permissions           │
                    │ if (m_sandboxManager) {                     │
                    │   if (!canAccess(absPath, Write)) {         │
                    │     return error: "Sandbox denied"          │
                    │   }                                         │
                    │ }                                           │
                    │                                             │
                    │ ⚠️  FAILURE POINT #12C:                     │
                    │  Sandbox manager not initialized            │
                    │  (nullptr) - skips permission check        │
                    │ ⚠️  FAILURE POINT #12D:                     │
                    │  Sandbox policy blocks write access         │
                    │                                             │
                    │ Step 4: Ensure parent directories           │
                    │ - Create /workspace/ if missing             │
                    │ - If mkdir fails: return error              │
                    │                                             │
                    │ ⚠️  FAILURE POINT #12E:                     │
                    │  Insufficient file system permissions       │
                    │  (can't create parent directory)            │
                    │                                             │
                    │ Step 5-6: Atomic write with QSaveFile       │
                    │ - Open file for writing                     │
                    │ - Write content via QTextStream             │
                    │ - Flush data                                │
                    │ - Commit atomically                         │
                    │                                             │
                    │ ⚠️  FAILURE POINT #12F:                     │
                    │  QSaveFile::open() fails                    │
                    │  (disk full, read-only filesystem, etc)    │
                    │ ⚠️  FAILURE POINT #12G:                     │
                    │  QSaveFile::commit() fails                  │
                    │  (atomic write failed)                      │
                    │                                             │
                    │ Return: ToolResult {                        │
                    │   id: callId                                │
                    │   content: "File written successfully"      │
                    │   isError: false                            │
                    │ }                                           │
                    │ }                                           │
                    └────────────┬────────────────────────────────┘
                                 │
                                 ▼
                    ┌─────────────────────────────────────────────┐
                    │ PHASE 8: RESULT HANDLING                    │
                    │                                             │
                    │ resultsMsg.toolResults.append(result)       │
                    │                                             │
                    │ if (!result.isError &&                      │
                    │     result.content.length() > 8000)         │
                    │ {                                           │
                    │   Summarize output (if needed)              │
                    │ }                                           │
                    │                                             │
                    │ appendMessage(resultsMsg)                   │
                    │ emit toolFinished(result)                   │
                    │                                             │
                    │ ⚠️  FAILURE POINT #13:                      │
                    │  toolFinished signal not connected          │
                    │  to update UI (no success/error shown)     │
                    │                                             │
                    │ Log output (if any)                         │
                    │                                             │
                    └────────────┬────────────────────────────────┘
                                 │
                                 ▼
                    ┌─────────────────────────────────────────────┐
                    │ PHASE 9: AGENT LOOP CONTINUES               │
                    │                                             │
                    │ Next iteration of runLoop():                │
                    │ - LLM sees tool result in history           │
                    │ - Generates next response (more tools or    │
                    │   final message)                            │
                    │ - Repeat from Phase 1                       │
                    │                                             │
                    │ When LLM says "I'm done":                   │
                    │ - m_verifier.turnComplete() → true          │
                    │ - Break from loop                           │
                    │ - emit turnComplete()                       │
                    │ - Set status to IDLE                        │
                    │                                             │
                    └─────────────────────────────────────────────┘
```

---

## Part 2: Failure Point Details Matrix

| # | Phase | Failure Point | Root Cause | Detection Method | Fix |
|---|-------|---------------|-----------|----|-----|
| 1 | LLM Generation | Tool name mismatch | LLM generates "write_file" but only "Write" registered | Check LLMRequest shows correct tool schemas | Register all variant names in registry |
| 2 | Tool Call Extraction | toolCalls array empty | LLM didn't generate WriteTool call | Check response.message.toolCalls.size() == 0 | Refine LLM prompt/system message |
| 3 | Approval Check | autoApproveTools = false but no UI signal connection | AgentEngine not connected to AgentController | Check AgentController::onToolApprovalRequired slot exists | Verify connect() in AgentController ctor |
| 4 | Approval Check | ApprovalManager nullptr | setApprovalManager() never called | Check m_approvalManager in debugger | Ensure initialization in AgentController::init() |
| 5 | Approval Signal | toolApprovalRequired signal not connected | QML not connected to agent.onToolApprovalRequired | Check signal connections with Qt Spy | Add connect() in main.cpp or AgentController |
| 6 | Approval Signal | ToolApprovalDialog not shown to user | QML not rendering dialog (visibility: hidden) | Check dialog appears at all; inspect z-order | Debug QML visibility logic |
| 7 | User Approval | User never sees approval dialog | Signal chain broken somewhere | Run agent with logging: grep -i "approval\|request" | Trace signal flow with qDebug |
| 8 | User Approval | User clicks approve but approveTool() not called | QML button not connected to agent.approveTool() | Check ToolApprovalDialog button onClick handler | Verify signal-slot connection in QML |
| 9 | Tool Execution | toolExecuting signal not connected to UI | AgentController::onToolExecuting not wired up | Check is signal being emitted? | Add connect() or implement handler in QML |
| 10 | Executor | WriteTool not registered in AgentToolRegistry | AgentController::initializeToolRegistry() never called or incomplete | Check m_registry->allTools().size() | Verify registry initialization |
| 11 | Executor | Wrong tool name used | LLM uses "write_file" but registered as "Write" | grep "name() const" ClaudeStandardTools.cpp | Ensure LLM sees correct schema names |
| 12A | WriteTool::execute | file_path parameter is empty | LLM generates WriteTool with empty file_path | Check call.arguments["file_path"].toString() | Improve LLM prompt |
| 12B | WriteTool::execute | Path traversal attack detected | safePath() returned empty string | Check logs for "Path traversal attack detected" | Review path validation logic |
| 12C | WriteTool::execute | Sandbox manager not initialized | setSandboxManager() never called | Check m_sandboxManager != nullptr | Verify sandbox setup in AgentController |
| 12D | WriteTool::execute | Sandbox policy denies write | SandboxManager::canAccess() returns false | Check sandbox logs | Review/relax sandbox policy |
| 12E | WriteTool::execute | Parent directory creation fails | ensureDirectoryExists() returns false | Check file system permissions | Ensure workspace is writable |
| 12F | WriteTool::execute | QSaveFile::open() fails | File can't be opened (disk full, read-only, etc) | Check save.errorString() in logs | Check disk space and file permissions |
| 12G | WriteTool::execute | QSaveFile::commit() fails | Atomic write operation failed | Check "QSaveFile commit failed" in logs | Check disk space and file permissions |
| 13 | Result Handling | toolFinished signal not connected | AgentController::onToolFinished not wired | Check signal-slot in debugger | Verify connect() in AgentController |

---

## Part 3: Configuration Checklist

### 1. **Tool Registration**
```cpp
// In AgentController::initializeToolRegistry()
auto* writeTool = new WriteTool(workspaceRoot);
writeTool->setSandboxManager(sandboxManager);
m_registry->registerTool(writeTool);
```

**Check:**
- [ ] WriteTool instantiated
- [ ] SandboxManager assigned
- [ ] Registered in registry with name "Write"
- [ ] All other tools registered

### 2. **Approval System Initialization**
```cpp
// In AgentController::init()
auto* approvalManager = new ApprovalManager();
m_engine->setApprovalManager(approvalManager);

AgentEngineConfig config;
config.autoApproveTools = false;  // ← Set based on UX requirement
m_engine->setConfig(config);
```

**Check:**
- [ ] ApprovalManager instantiated
- [ ] setApprovalManager() called
- [ ] autoApproveTools set appropriately
- [ ] Default policy configured

### 3. **Signal Connections**
```cpp
// In AgentController::init()
connect(m_engine, &AgentEngine::toolApprovalRequired,
        this, &AgentController::onToolApprovalRequired);
connect(m_engine, &AgentEngine::toolExecuting,
        this, &AgentController::onToolExecuting);
connect(m_engine, &AgentEngine::toolFinished,
        this, &AgentController::onToolFinished);
```

**Check:**
- [ ] toolApprovalRequired → QML update
- [ ] toolExecuting → show loading
- [ ] toolFinished → update results
- [ ] All slots implemented

### 4. **QML Connections**
```qml
// In ChatView.qml or similar
Connections {
    target: agent
    onToolApprovalRequired: {
        toolApprovalDialog.show(call.id, call.name, ...)
    }
}
```

**Check:**
- [ ] ToolApprovalDialog connected
- [ ] Approve button calls agent.approveTool(callId, true)
- [ ] Reject button calls agent.approveTool(callId, false)
- [ ] Dialog visibility controlled

### 5. **Logging & Debugging**
Add to WriteTool::execute() and see in application output:
```
[WriteTool] <callId> START: file_path=... content_size=...
[WriteTool] <callId> Step 1: Resolved absolute path: ...
[WriteTool] <callId> Step 2: Sandbox permission check PASSED
[WriteTool] <callId> Step 3: Parent directory ensured
[WriteTool] <callId> Step 4: File opened for writing
[WriteTool] <callId> Step 5: Content flushed to stream
[WriteTool] <callId> Step 6: File committed atomically
```

---

## Part 4: Quick Diagnosis Script

### For Developers:

1. **Check Tool Registry:**
```cpp
qDebug() << "Registered tools:" << m_registry->allTools().size();
for (auto tool : m_registry->allTools()) {
    qDebug() << "  -" << tool->name();
}
```

Expected output:
```
Registered tools: 8
  - Write
  - Edit
  - MultiEdit
  - Read
  - ReadTree
  - Bash
  - Grep
  - Glob
```

2. **Check Tool Call:**
```cpp
qDebug() << "Tool call name:" << call.name << "ID:" << call.id;
qDebug() << "Arguments:" << QJsonDocument(call.arguments).toJson();
```

3. **Check Approval Flow:**
In main.cpp after AgentEngine created:
```cpp
connect(m_engine, &AgentEngine::toolApprovalRequired,
        [](const ToolCall &call, const QString &risk) {
            qDebug() << "APPROVAL REQUIRED:" << call.name << "Risk:" << risk;
        });
```

4. **Check Executor:**
In Executor::execute():
```cpp
BaseTool *tool = m_registry ? m_registry->tool(call.name) : nullptr;
qDebug() << "Tool lookup:" << call.name << "→" << (tool ? tool->name() : "NOT FOUND");
```

---

## Part 5: User-Facing Checklist

### If write doesn't happen:

1. **Does approval dialog appear?**
   - Yes → User needs to click "Approve" or "Reject"
   - No → Approval signal chain broken (Failure Points #5-6)

2. **Does "Approve" button work?**
   - Yes → Check logs for "Step 1" WriteTool messages
   - No → QML button not connected (Failure Point #8)

3. **Do you see error message?**
   - Yes → Read error message (Failure Points #12A-12G)
   - No → toolFinished signal not connected (Failure Point #13)

4. **Did agent say "I'm done"?**
   - Yes → Write succeeded or failed, check file system
   - No → Agent loop still waiting (approval not processed)

---

## Part 6: Key Code Locations

| Component | File | Key Functions |
|-----------|------|---|
| **WriteTool** | `src/tools/ClaudeStandardTools.cpp` | `WriteTool::execute()` |
| **Executor** | `src/agent/Executor.cpp` | `Executor::execute()` |
| **ApprovalCheck** | `src/agent/AgentEngine.cpp` | `shouldRequireApproval()`, `approvalRiskLevel()` |
| **ApprovalSignal** | `src/agent/AgentEngine.cpp` | `approveTool()`, line 651+ |
| **Tool Registry** | `src/agent/AgentToolRegistry.cpp` | `registerTool()`, `tool()` |
| **AgentController** | `src/bridge/AgentController.cpp` | `onToolApprovalRequired()`, `onToolFinished()` |
| **QML Dialog** | `content/ToolApprovalDialog.qml` | `approved()`, `rejected()` signals |
| **Initialization** | `src/main.cpp` | Signal connections |

---

## Part 7: Summary of Silent Failures

These failures result in **NO ERROR MESSAGE** shown to user:

1. **Approval signal not connected** → Dialog never shown, user waits forever
2. **WriteTool not registered** → Agent shows error "Unknown tool: Write"
3. **Tool not executed after approval** → Dialog closes but nothing happens
4. **Sandbox silently blocks** → Write silently fails, error message shown
5. **toolFinished signal not connected** → Result not displayed in UI

---

## Part 8: Recovery & Testing

### Unit Test WriteTool Directly:
```cpp
WriteTool tool("/workspace");
QJsonObject args;
args["file_path"] = "test.txt";
args["new_text"] = "Hello World";

ToolResult result = tool.execute("test-1", args);
qDebug() << "Success:" << !result.isError << "Content:" << result.content;
```

### End-to-End Test:
1. Enable autoApproveTools = true temporarily
2. Send: "Create file hello.txt with 'test' content"
3. Check file system for hello.txt
4. Verify content with: cat hello.txt

### Approval Test:
1. Set autoApproveTools = false
2. Send: "Create file test2.txt with content"
3. Verify ToolApprovalDialog appears
4. Click "Approve"
5. Verify file created

---

## Conclusion

The write operation flow involves 13 distinct phases and 19+ potential failure points. The most common issues are:

1. **Approval signal chain broken** (Failures #5-8)
2. **Tool not registered** (Failure #10)
3. **Result signal not connected** (Failure #13)

Use the diagrams and checklists in this guide to systematically identify and fix the problem.
