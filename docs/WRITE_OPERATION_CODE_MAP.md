# Write Operation - Complete Code Map & References

**Last Updated:** 2026-06-12  
**Scope:** neurx-code (primary), hermes-agent  
**Quick Reference for Developers**

---

## File Structure & Key Functions

```
neurx-code/
├── src/
│   ├── main.cpp
│   │   └── Signal connections [Line ~150-200]
│   │       • connect(agent, toolApprovalRequired, ...)
│   │       • connect(agent, toolExecuting, ...)
│   │       • connect(agent, toolFinished, ...)
│   │
│   ├── bridge/
│   │   ├── AgentController.h
│   │   │   ├── Q_PROPERTY(autoApproveTools)
│   │   │   ├── Q_INVOKABLE void approveTool(QString callId, bool approved)
│   │   │   ├── Q_SLOT void onToolApprovalRequired(ToolCall, QString risk)
│   │   │   ├── Q_SLOT void onToolExecuting(ToolCall)
│   │   │   └── Q_SLOT void onToolFinished(ToolResult)
│   │   │
│   │   └── AgentController.cpp
│   │       ├── init() → initializeToolRegistry()
│   │       ├── initializeToolRegistry() [~Line 3750-3850]
│   │       │   └── registerTool(writeTool) ← KEY: must register "Write"
│   │       ├── onToolApprovalRequired() [~Line 3906+]
│   │       │   └── Emits signal to QML dialog
│   │       ├── approveTool() [~Line ?]
│   │       │   └── Calls m_engine->approveTool()
│   │       ├── onToolExecuting()
│   │       │   └── Updates UI with loading state
│   │       └── onToolFinished()
│   │           └── Updates UI with result
│   │
│   ├── agent/
│   │   ├── AgentEngine.h
│   │   │   ├── Q_PROPERTY(status) - Idle/Thinking/Executing/Waiting
│   │   │   ├── Q_SIGNAL void toolApprovalRequired(ToolCall, QString)
│   │   │   ├── Q_SIGNAL void toolExecuting(ToolCall)
│   │   │   ├── Q_SIGNAL void toolFinished(ToolResult)
│   │   │   ├── Q_SLOT void approveTool(QString callId, bool approved)
│   │   │   ├── Q_SLOT void submitUserMessage(QString)
│   │   │   ├── setAutoApproveTools(bool)
│   │   │   ├── setApprovalManager(ApprovalManager*)
│   │   │   └── setToolRegistry(AgentToolRegistry*)
│   │   │
│   │   ├── AgentEngine.cpp
│   │   │   ├── Constructor() [~Line 100+]
│   │   │   │   └── Initialize managers (EventBus, SlashCommandManager, etc)
│   │   │   │
│   │   │   ├── submitUserMessage() [~Line 550+]
│   │   │   │   ├── Append message
│   │   │   │   ├── setStatus(Thinking)
│   │   │   │   └── QtConcurrent::run(runLoop)
│   │   │   │
│   │   │   ├── runLoop() [~Line 675+]  ← MAIN AGENT LOOP
│   │   │   │   │
│   │   │   │   ├─ Phase 1: Send LLM request
│   │   │   │   │  └── LLM returns response with toolCalls
│   │   │   │   │
│   │   │   │   ├─ Phase 2: Process tool calls
│   │   │   │   │  for (const auto &call : response.toolCalls) {
│   │   │   │   │    if (shouldRequireApproval(call)) {
│   │   │   │   │      emit toolApprovalRequired(call, risk)
│   │   │   │   │      WAIT in: m_pendingApprovals[callId]
│   │   │   │   │    }
│   │   │   │   │    emit toolExecuting(call)
│   │   │   │   │    result = m_executor.execute(call)
│   │   │   │   │    emit toolFinished(result)
│   │   │   │   │  }
│   │   │   │   │
│   │   │   │   └─ Phase 3: Loop or return based on verifier
│   │   │   │
│   │   │   ├── shouldRequireApproval(ToolCall) [~Line 600+] ← APPROVAL LOGIC
│   │   │   │   ├─ Check ApprovalManager policy
│   │   │   │   ├─ Check ExecutionStrategy
│   │   │   │   ├─ Check autoApproveTools
│   │   │   │   └─ Return: bool
│   │   │   │
│   │   │   ├── approvalRiskLevel(ToolCall) [~Line 565+]
│   │   │   │   └─ Return: "low"/"medium"/"high"/"critical"
│   │   │   │
│   │   │   ├── approveTool(QString callId, bool approved) [~Line 651+]
│   │   │   │   ├─ QMutexLocker locker(&m_mutex)
│   │   │   │   ├─ if (!approved) → emit result with "denied"
│   │   │   │   └─ m_pendingApprovals.remove(callId) ← UNBLOCK runLoop
│   │   │   │
│   │   │   └── Private members:
│   │   │       ├─ AgentToolRegistry *m_registry
│   │   │       ├─ ApprovalManager *m_approvalManager
│   │   │       ├─ Executor m_executor
│   │   │       ├─ QHash<QString, bool> m_pendingApprovals
│   │   │       └─ AgentEngineConfig m_config
│   │   │
│   │   ├── Executor.h
│   │   │   └── ToolResult execute(const ToolCall &call)
│   │   │
│   │   ├── Executor.cpp
│   │   │   └── Executor::execute(const ToolCall &call) [~Line 11+]
│   │   │       ├─ BaseTool *tool = m_registry->tool(call.name)
│   │   │       ├─ if (!tool) → error "Unknown tool"
│   │   │       └─ return tool->execute(call.id, call.arguments)
│   │   │
│   │   ├── AgentToolRegistry.h
│   │   │   ├── void registerTool(BaseTool *tool)
│   │   │   ├── BaseTool *tool(const QString &name) const
│   │   │   └── QList<BaseTool*> allTools() const
│   │   │
│   │   └── AgentToolRegistry.cpp
│   │       └── Maps tool name → BaseTool pointer
│   │
│   └── tools/
│       ├── ClaudeStandardTools.h [~Line 31-45]
│       │   └── class WriteTool : public BaseTool {
│       │       ├── QString name() const override { return "Write"; }
│       │       ├── ToolResult execute(callId, args) override
│       │       ├── void setSandboxManager(SandboxManager*)
│       │       └── Private:
│       │           ├── QString safePath(QString relPath)
│       │           ├── bool ensureDirectoryExists(QString dir)
│       │           └── SandboxManager *m_sandboxManager
│       │
│       └── ClaudeStandardTools.cpp [~Line 112-200+]
│           └── WriteTool::execute(callId, args)
│               ├─ Step 1: Validate inputs
│               ├─ Step 2: Resolve path (safePath)
│               ├─ Step 3: Check sandbox permissions
│               ├─ Step 4: Ensure parent directories
│               ├─ Step 5: Open file with QSaveFile
│               ├─ Step 6: Write content via QTextStream
│               ├─ Step 7: Commit atomically
│               └─ Return: ToolResult { isError, content }
│
└── content/
    └── ToolApprovalDialog.qml
        ├── function show(callId, name, summary, risk, reason)
        ├── signal approved(callId)
        ├── signal rejected(callId)
        ├── Button "Approve"
        │   └── onClicked: agent.approveTool(pendingCallId, true)
        └── Button "Reject"
            └── onClicked: agent.approveTool(pendingCallId, false)
```

---

## Phase-by-Phase Code Execution

### Phase 1: User Submits Message
```cpp
// FILE: src/bridge/AgentController.cpp
void AgentController::sendUserMessage(const QString &text) {
    // ...
    m_engine->submitUserMessage(text);  // → Calls AgentEngine
}

// FILE: src/agent/AgentEngine.cpp
void AgentEngine::submitUserMessage(const QString &text, ...) {
    AgentMessage userMsg;
    userMsg.role = MessageRole::User;
    userMsg.content = text;
    appendMessage(userMsg);
    
    m_interrupted = false;
    setStatus(AgentStatus::Thinking);
    
    const auto future = QtConcurrent::run([this]() { runLoop(); });
}
```

### Phase 2: Agent Loop Receives LLM Response
```cpp
// FILE: src/agent/AgentEngine.cpp (in runLoop)
// ~Line 675
while (iterations++ < m_config.maxIterations && !m_interrupted) {
    // Build request...
    LLMResponse response = m_provider->sendRequest(req);
    
    // Response contains: toolCalls = [ToolCall{name:"Write", id:"call-123", args:{...}}]
    appendMessage(response.message);
    
    if (m_verifier.turnComplete(response.message)) {
        break;  // No tool calls, LLM finished
    }
    
    // ← Tool calls exist, process them...
}
```

### Phase 3: Check if Tool Needs Approval
```cpp
// FILE: src/agent/AgentEngine.cpp (in runLoop)
for (const auto &call : response.message.toolCalls) {
    
    // APPROVAL CHECK
    if (shouldRequireApproval(call)) {  // ← Line ~800
        qInfo() << "[agent] Approval required for:" << call.name;
        
        // Add to pending approvals
        {
            QMutexLocker locker(&m_mutex);
            m_pendingApprovals.insert(call.id);  // ← Marks as "waiting"
        }
        
        // Emit approval signal (connects to QML UI)
        emit toolApprovalRequired(call, approvalRiskLevel(call));  // ← Line 826
        
        // WAIT for approval (blocked by m_pendingApprovals)
        setStatus(AgentStatus::Waiting);
        
        // Loop will continue when approveTool() removes call.id
        // from m_pendingApprovals
        continue;  // ← Skip execution for now
    }
    
    // Tool approved (or auto-approve), execute it
    emit toolExecuting(call);
    ToolResult result = m_executor.execute(call);
    emit toolFinished(result);
}
```

### Phase 4: User Approves in QML
```qml
// FILE: content/ToolApprovalDialog.qml
Button {
    text: "Approve"
    onClicked: {
        root.approved(pendingCallId)  // → emits signal
        agent.approveTool(pendingCallId, true)  // ← Calls C++
        root.close()
    }
}

// FILE: src/bridge/AgentController.h
Q_INVOKABLE void approveTool(const QString &callId, bool approved) {
    if (m_engine) {
        m_engine->approveTool(callId, approved);
    }
}
```

### Phase 5: Unblock Agent Loop
```cpp
// FILE: src/agent/AgentEngine.cpp
void AgentEngine::approveTool(const QString &callId, bool approved) {
    QMutexLocker locker(&m_mutex);
    
    if (!m_pendingApprovals.contains(callId)) return;
    
    if (!approved) {
        m_pendingApprovals.remove(callId);
        ToolResult denied{callId, "", true, "Tool execution denied by user."};
        appendMessage(toolResultMsg);
        return;
    }
    
    // APPROVED: Remove from pending
    m_pendingApprovals.remove(callId);  // ← UNBLOCKS runLoop
}

// Back in runLoop(), the execution continues...
```

### Phase 6: Execute WriteTool
```cpp
// FILE: src/agent/Executor.cpp
ToolResult Executor::execute(const ToolCall &call) const {
    BaseTool *tool = m_registry ? m_registry->tool(call.name) : nullptr;
    
    if (!tool) {
        return {call.id, call.name, true, "Unknown tool: " + call.name};
    }
    
    ToolResult result = tool->execute(call.id, call.arguments);
    
    // Summarize large output if needed
    if (!result.isError && result.content.length() > 8000) {
        result.content = m_summarizer.summarize(call.name, result.content);
    }
    
    return result;
}

// FILE: src/tools/ClaudeStandardTools.cpp
ToolResult WriteTool::execute(const QString& callId, const QJsonObject& args) {
    QString filePath = args.value("file_path").toString();
    QString newText = args.value("new_text").toString();
    
    qInfo() << "[WriteTool]" << callId << "START: file_path=" << filePath;
    
    // Step 1: Validate
    if (filePath.isEmpty()) {
        return {callId, name(), true, "Error: file_path is empty"};
    }
    
    // Step 2: Resolve path
    QString absPath = safePath(filePath);  // ← Prevents ../../../ attacks
    if (absPath.isEmpty()) {
        return {callId, name(), true, "Error: Path traversal attack detected"};
    }
    
    qInfo() << "[WriteTool]" << callId << "Step 1: Resolved:" << absPath;
    
    // Step 3: Sandbox check
    if (m_sandboxManager) {
        if (!m_sandboxManager->canAccess(absPath, FileSystemAccessMode::Write)) {
            return {callId, name(), true, "Error: Sandbox denied write access"};
        }
        qInfo() << "[WriteTool]" << callId << "Step 2: Sandbox check PASSED";
    }
    
    // Step 4: Create parent directories
    QFileInfo fileInfo(absPath);
    QString parentDir = fileInfo.dir().absolutePath();
    if (!ensureDirectoryExists(parentDir)) {
        return {callId, name(), true, "Error: Failed to create parent directories"};
    }
    qInfo() << "[WriteTool]" << callId << "Step 3: Parent directory ensured";
    
    // Step 5-6: Atomic write
    QSaveFile save(absPath);
    if (!save.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return {callId, name(), true, "Error: Cannot open file for writing"};
    }
    qInfo() << "[WriteTool]" << callId << "Step 4: File opened";
    
    QTextStream out(&save);
    out.setEncoding(QStringConverter::Utf8);
    out << newText;
    out.flush();
    qInfo() << "[WriteTool]" << callId << "Step 5: Content flushed";
    
    if (!save.commit()) {
        save.cancelWriting();
        return {callId, name(), true, "Error: Failed to write file atomically"};
    }
    qInfo() << "[WriteTool]" << callId << "Step 6: File committed";
    
    return {callId, name(), false, "File written successfully"};
}
```

### Phase 7: Result Handling
```cpp
// Back in runLoop()
resultsMsg.toolResults.append(result);

if (!result.isError && result.content.length() > 8000) {
    result.content = m_summarizer.summarize(call.name, result.content);
}

appendMessage(resultsMsg);
emit toolFinished(result);  // ← Signal to QML UI

// If m_verifier.turnComplete(), break loop
// Otherwise, loop again for next LLM request with tool result
```

---

## Signal Connection Chain

```
┌─ AgentEngine ────────────────────────────────┐
│ SIGNAL: toolApprovalRequired(ToolCall, str)  │
└──────────────────┬──────────────────────────┘
                   │ [MUST BE CONNECTED]
                   ▼
┌─ AgentController ─────────────────────────────────────────┐
│ SLOT: onToolApprovalRequired(ToolCall, QString) {         │
│   Q_EMIT toolApprovalRequiredChanged(...)                │
│ }                                                         │
└──────────────────┬──────────────────────────────────────┘
                   │ [Q_SIGNAL]
                   ▼
┌─ QML Context ─────────────────────────────────────┐
│ agent.onToolApprovalRequired(call, risk)          │
│ → ToolApprovalDialog.show(callId, name, ...)     │
└──────────────────┬───────────────────────────────┘
                   │ [User clicks "Approve"]
                   ▼
┌─ QML Button ────────────────────────────────────┐
│ onClicked: {                                    │
│   agent.approveTool(pendingCallId, true)       │
│ }                                              │
└──────────────────┬────────────────────────────┘
                   │ [Q_INVOKABLE]
                   ▼
┌─ AgentController ───────────────────────────────┐
│ Q_INVOKABLE void approveTool(callId, approved) │
│   m_engine->approveTool(callId, approved)      │
└──────────────────┬────────────────────────────┘
                   │
                   ▼
┌─ AgentEngine ───────────────────────────────────┐
│ void approveTool(callId, approved) {           │
│   m_pendingApprovals.remove(callId)            │
│   // ← Unblocks runLoop                        │
│ }                                              │
└─────────────────────────────────────────────────┘
```

---

## Configuration Objects

### AgentEngineConfig
```cpp
// FILE: src/agent/AgentEngine.h
struct AgentEngineConfig {
    QString systemPrompt;                    // System prompt for LLM
    int     maxIterations{15};               // Max agent loop iterations
    int     contextWindowTokens{65536};      // LLM context budget
    int     maxCompletionTokens{4096};       // Max output tokens
    bool    autoApproveTools{false};         // Auto-approve tools or wait for user?
};
```

### ApprovalManager
```cpp
// FILE: src/approvals/ApprovalManager.h
class ApprovalManager {
    AskForApproval getPolicyFor(const QString &toolName, const QString &resource);
    bool isReadOnlyMode() const;
    QList<PendingApproval> getPendingApprovals() const;
    ApprovalPolicy getDefaultPolicy() const;
};

enum class AskForApproval {
    Never,              // Never ask, auto-approve
    OnRequest,          // Ask every time
    Granular,          // Ask based on resource type
    UnlessTrusted,     // Ask unless folder is trusted
    OnFailure          // Ask only on failure
};
```

---

## Debugging Aids

### Enabling Detailed Logging
```cpp
// In main.cpp before creating engine:
#ifdef QT_DEBUG
QLoggingCategory::setFilterRules(QStringLiteral("*.debug=true"));
#endif
```

### Adding Debug Output
```cpp
// To WriteTool::execute():
qDebug() << "[WriteTool] CALLED" << callId 
         << "file=" << filePath 
         << "size=" << newText.length();

// To AgentEngine::runLoop():
qDebug() << "[agent] Tool call:" << call.name 
         << "requires approval:" << shouldRequireApproval(call);

// To Executor::execute():
qDebug() << "[executor] Looking up tool:" << call.name 
         << "found:" << (tool ? "yes" : "no");
```

### Qt Creator Debugging
```gdb
# Launch with:
file neurx-code-debug
break WriteTool::execute
break AgentEngine::approveTool
break Executor::execute

# Watch variables:
watch m_pendingApprovals
watch m_config.autoApproveTools

# Print signals:
print m_engine->receivers(SIGNAL(toolApprovalRequired(ToolCall, QString)))
```

---

## Critical Line Numbers Reference

| Function | File | Lines | Purpose |
|----------|------|-------|---------|
| `runLoop()` | AgentEngine.cpp | 675-900 | Main agent loop |
| `shouldRequireApproval()` | AgentEngine.cpp | 600-620 | Approval logic |
| `approvalRiskLevel()` | AgentEngine.cpp | 565-590 | Risk assessment |
| `approveTool()` | AgentEngine.cpp | 651+ | Handle user approval |
| `toolApprovalRequired` signal | AgentEngine.cpp | 826 | Emit approval signal |
| `initializeToolRegistry()` | AgentController.cpp | 3750-3850 | Register tools |
| `WriteTool::execute()` | ClaudeStandardTools.cpp | 112+ | Execute write |
| `signal connections` | main.cpp | 150-200 | Connect signals |

---

## Common Mistakes to Avoid

1. **❌ Tool registered with wrong name**
   ```cpp
   m_registry->registerTool(writeTool);  // name() returns "Write"
   // But LLM calls: "write_file" or "codex_agent"
   ```
   **✓ Fix:** Ensure tool name matches LLM schema

2. **❌ Signal not connected**
   ```cpp
   // In main.cpp, forgot to add:
   // connect(engine, &AgentEngine::toolApprovalRequired, ...)
   ```
   **✓ Fix:** Add connection in main.cpp or AgentController ctor

3. **❌ SandboxManager not initialized**
   ```cpp
   auto* writeTool = new WriteTool(root);
   // Forgot: writeTool->setSandboxManager(sandboxManager);
   m_registry->registerTool(writeTool);
   ```
   **✓ Fix:** Always call setSandboxManager() before registration

4. **❌ QML button not connected**
   ```qml
   Button {
       text: "Approve"
       onClicked: {  // ← Missing: agent.approveTool()
           root.close()
       }
   }
   ```
   **✓ Fix:** Call agent.approveTool(callId, true)

5. **❌ autoApproveTools confusion**
   ```cpp
   config.autoApproveTools = false;  // Requires approval
   config.autoApproveTools = true;   // Skips approval dialog
   ```
   **✓ Remember:** false = ask user, true = auto-execute

---

Created: 2026-06-12 | For: developers debugging write operations
