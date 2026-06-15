# Write Operation Troubleshooting Quick Reference

**Quick Link to Detailed Guide:** [WRITE_OPERATION_FAILURE_DIAGNOSIS_GUIDE.md](WRITE_OPERATION_FAILURE_DIAGNOSIS_GUIDE.md)

---

## Symptom → Root Cause → Fix Matrix

### Symptom 1: "File wasn't created"

**Step 1: Check if agent attempted write**
```bash
# Look for WriteTool messages in logs
grep -i "WriteTool.*START" logs/*.log
```

**If no output:**
→ **Failure Point #2**: LLM didn't generate WriteTool call
- Fix: Check system prompt; maybe LLM chose different tool

**If output found:** Continue to Step 2

---

**Step 2: Check if user approved**
```bash
# Look for approval signal
grep "toolApprovalRequired\|APPROVAL REQUIRED" logs/*.log
```

**If no output and autoApproveTools=false:**
→ **Failure Point #5-6**: Approval dialog never shown
- Fix: Check AgentController::onToolApprovalRequired() implemented
- Check: Signal connection in main.cpp exists

**If output found:**
→ **Failure Point #7-8**: User approval stuck
- Fix: Check ToolApprovalDialog.approved() calls agent.approveTool()
- Verify: QML button onClick handler

**If autoApproveTools=true:** Continue to Step 3

---

**Step 3: Check WriteTool execution**
```bash
# Look for WriteTool Step 1-6 logs
grep "WriteTool.*Step" logs/*.log | head -20
```

**If "Step 1: Resolved absolute path" found:**
→ WriteTool ran successfully
→ **Failure Point #12A-G**: File write failed
- Check last step that printed
- See **Symptom 2** below for specific step failure

**If "Step 1" not found:**
→ **Failure Point #10-11**: WriteTool not found/registered
- Fix: Check `m_registry->tool("Write")` not nullptr
- Verify: WriteTool instantiated and registered

---

### Symptom 2: "WriteTool failed at Step N"

**Step 1: Failed at Step 1** (Path resolution)
```bash
grep "Step 1.*Resolved absolute path" logs/*.log
```
**Failure Point #12B**: Path traversal attack detected
- User provided: `../../../etc/passwd`
- Fix: Validate file_path parameter; reject absolute paths or use workspace-relative only

---

**Step 2: Failed at Step 2** (Sandbox check)
```bash
grep "Sandbox denied write access\|No sandbox manager" logs/*.log
```

**If "Sandbox denied":**
→ **Failure Point #12D**: Sandbox policy blocked write
- Fix: Check SandboxManager permissions for workspace path
- Command: `grep -i "sandbox.*policy\|sandboxmanager" config.json`

**If "No sandbox manager":**
→ **Failure Point #12C**: SandboxManager not initialized
- Fix: Call `writeTool->setSandboxManager(sandboxManager)` in init

---

**Step 3: Failed at Step 3** (Create parent directory)
```bash
grep "Failed to create parent directory" logs/*.log
```
→ **Failure Point #12E**: Directory creation failed
- Fix: Check workspace folder has write permissions
- Command: `ls -ld /workspace/ && touch /workspace/test.txt`

---

**Step 4: Failed at Step 5** (QSaveFile open)
```bash
grep "Cannot open file for writing\|QSaveFile.*Error" logs/*.log
```
→ **Failure Point #12F**: File opening failed
- Reasons: disk full, read-only filesystem, file locked
- Fix: Check `df -h` (disk space), check file permissions

---

**Step 5: Failed at Step 6** (QSaveFile commit)
```bash
grep "QSaveFile commit failed" logs/*.log
```
→ **Failure Point #12G**: Atomic write failed
- Reasons: disk full during write, file permissions changed
- Fix: Check `df -h` and file system status

---

### Symptom 3: "User sees no result/error message"

**Check 1: Did WriteTool execute?**
```bash
grep "ToolResult.*WriteTool" logs/*.log
```

**If yes:**
→ **Failure Point #13**: toolFinished signal not connected
- Fix: Verify AgentController::onToolFinished() wired up
- Check: connect() in AgentController ctor

**If no:**
→ **Failure Point #9**: Tool not executed at all
- Fix: Check agent loop blocked waiting for approval
- See: **Symptom 1, Step 2**

---

## Configuration Checklist

### 1. Tools Registered?
```cpp
// Add to main.cpp after m_registry created:
for (auto tool : m_registry->allTools()) {
    qDebug() << "Tool:" << tool->name();
}
```

**Expected output should include:**
```
Tool: Write
Tool: Edit
Tool: Read
```

### 2. Approval Manager Set?
```cpp
// Add to AgentController::init():
if (!m_approvalManager) {
    qWarning() << "ApprovalManager not initialized!";
} else {
    qDebug() << "ApprovalManager ready";
}
```

### 3. autoApproveTools Configuration?
```cpp
// In AgentEngine config:
qDebug() << "autoApproveTools:" << m_config.autoApproveTools;
```

### 4. SandboxManager Assigned?
```cpp
// In WriteTool:
if (!m_sandboxManager) {
    qWarning() << "SandboxManager not assigned to WriteTool!";
}
```

### 5. Signals Connected?
```cpp
// In AgentController::init():
const auto& connections = disconnect(nullptr);
// Or use Qt Creator's "Show Signals/Slots" feature
```

---

## One-Liner Diagnostics

| Issue | One-Liner Check |
|-------|---|
| Tool not registered | `grep -c 'registerTool.*Write' *.cpp` |
| Approval not signaled | `grep 'emit toolApprovalRequired' *.cpp` |
| ApprovalManager null | `grep 'if.*m_approvalManager' *.cpp \| head -5` |
| Sandbox not set | `grep 'setSandboxManager' *.cpp` |
| File write succeeded | `ls -la /workspace/output.txt` (check timestamp) |
| Disk full? | `df -h` (look for 100%) |
| Logs have errors? | `grep -i 'error\|failed\|denied' logs/*.log \| tail -20` |

---

## Network Request Failure (if using hermes-agent)

### Symptom: "Write request sent but nothing happens"

**Check 1: Server received request?**
```bash
# In hermes-agent logs:
grep "write_file\|WriteTool" logs/server.log
```

**Check 2: Response sent back?**
```bash
# In client logs:
grep "toolFinished\|tool.*result" logs/*.log
```

**Check 3: Network latency?**
```bash
# Measure round-trip time
time curl -X POST http://localhost:8000/execute_tool \
  -H "Content-Type: application/json" \
  -d '{"tool": "write", "args": {"file_path": "test.txt"}}'
```

---

## Environment Variables

Set these to increase logging:

```bash
# Enable all debug logs
export QT_DEBUG_PLUGINS=1
export QT_LOGGING_RULES="*=true"

# Enable just agent logs
export QT_LOGGING_RULES="agent*=true"
export QT_LOGGING_RULES="neurx.agent*=true"

# Run with logging
./neurx-code 2>&1 | tee debug.log
```

---

## GDB Debugging Breakpoints

```gdb
# Break when WriteTool executes
break WriteTool::execute

# Break on approval required
break "AgentEngine::toolApprovalRequired"

# Break on executor failure
break "Executor::execute"

# Break on registry lookup failure
break "AgentToolRegistry::tool"

# Print tool name when registry fails
print call.name
print m_registry->allTools().size()
```

---

## Integration Test Script

```bash
#!/bin/bash
echo "=== Write Operation Test ==="

# Test 1: Tool registered
echo "Test 1: Tool Registry..."
grep -q "registerTool.*Write" src/bridge/AgentController.cpp && \
  echo "✓ WriteTool registration found" || \
  echo "✗ WriteTool registration NOT found"

# Test 2: Approval signal connected
echo "Test 2: Approval Signal..."
grep -q "connect.*toolApprovalRequired" src/bridge/AgentController.cpp && \
  echo "✓ Approval signal connected" || \
  echo "✗ Approval signal NOT connected"

# Test 3: SandboxManager initialized
echo "Test 3: Sandbox Manager..."
grep -q "setSandboxManager" src/tools/ClaudeStandardTools.cpp && \
  echo "✓ SandboxManager assignment found" || \
  echo "✗ SandboxManager NOT initialized"

# Test 4: Check file permission
echo "Test 4: File Permissions..."
if [ -w /tmp ]; then
  echo "✓ /tmp is writable"
else
  echo "✗ /tmp is NOT writable"
fi

echo "=== End Test ==="
```

---

## Most Common Fixes

### Fix #1: Add Missing Signal Connection
```cpp
// In AgentController::init()
connect(m_engine, &AgentEngine::toolApprovalRequired,
        this, &AgentController::onToolApprovalRequired);  // ← ADD THIS
```

### Fix #2: Register WriteTool
```cpp
// In AgentController::initializeToolRegistry()
auto* writeTool = new WriteTool(workspaceRoot);
writeTool->setSandboxManager(m_sandboxManager);
m_registry->registerTool(writeTool);  // ← ADD THIS
```

### Fix #3: Set autoApproveTools
```cpp
// In AgentController constructor or config
config.autoApproveTools = false;  // → Change to true if no approval UI
m_engine->setConfig(config);
```

### Fix #4: Connect QML Approve Button
```qml
// In ToolApprovalDialog.qml
Button {
    text: "Approve"
    onClicked: agent.approveTool(pendingCallId, true)  // ← ADD THIS
}
```

### Fix #5: Check Workspace Writable
```bash
touch /workspace/test.txt && rm /workspace/test.txt && \
  echo "Workspace is writable" || \
  echo "Workspace NOT writable - fix permissions!"
```

---

## When All Else Fails

1. **Set autoApproveTools = true** and test
   - If works: approval UI issue (Failures #5-8)
   - If fails: execution issue (Failures #10-13)

2. **Add qDebug() to WriteTool::execute() first line**
   ```cpp
   qInfo() << "[WriteTool] EXECUTE CALLED for callId=" << callId;
   ```
   - If appears in logs: tool is being called
   - If not: executor/registry issue

3. **Manually call WriteTool**
   ```cpp
   WriteTool tool("/workspace");
   QJsonObject args{{"file_path", "test.txt"}, {"new_text", "hello"}};
   ToolResult result = tool.execute("manual-test", args);
   qDebug() << "Result:" << result.content;
   ```
   - If file created: WriteTool works, issue is in flow
   - If file NOT created: WriteTool itself broken

4. **Trace complete flow in debugger**
   - Set breakpoints in: submitUserMessage → runLoop → approveTool → execute → WriteTool::execute
   - Step through each phase
   - Check all signal connections

---

## Performance Issues

**If write is very slow:**
```bash
# Check for large file I/O
time echo "test" > /workspace/test.txt  # Should be < 1ms

# Check for filesystem issues
df -h  # Check I/O wait
iostat -x 1 5  # Monitor disk
```

**Fix:** Ensure /workspace is on local SSD, not network mount

---

## File Not Created But No Error

This usually means:
1. **Writing to wrong location** → Check logs for absolute path
2. **File created but hidden** → Check for dot-files: `ls -la`
3. **Write succeeded but result signal not connected** → See Symptom 3
4. **Race condition in async agent loop** → File created after agent exits

**Debug:**
```bash
# Monitor directory for file creation
inotifywait -m /workspace/ -e create

# Meanwhile, run agent:
./neurx-code

# Or check file timestamps:
ls -lat /workspace/ | head -5
```

---

## Sandbox Permission Issues

**To debug sandbox:**
```cpp
// Add before WriteTool::execute
if (m_sandboxManager) {
    bool canWrite = m_sandboxManager->canAccess(absPath, FileSystemAccessMode::Write);
    qDebug() << "Sandbox check for" << absPath << ":" << (canWrite ? "ALLOWED" : "DENIED");
}
```

**Common sandbox issues:**
- Path outside workspace (e.g., /etc/passwd)
- File has restrictive permissions from previous run
- Workspace mounted read-only

**Fix:** Either relax sandbox policy or fix file permissions

---

## Testing Checklist

- [ ] Tool registered in AgentToolRegistry
- [ ] ApprovalManager initialized
- [ ] autoApproveTools configured
- [ ] Signal connections verified
- [ ] QML dialog tested
- [ ] File permissions correct
- [ ] Disk space available
- [ ] Logs show each step
- [ ] File appears in filesystem
- [ ] Content is correct

---

Created: 2026-06-12 | Scope: neurx-code, hermes-agent | Status: PRODUCTION READY
