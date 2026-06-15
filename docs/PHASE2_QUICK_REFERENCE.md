# Phase 2 Quick Reference Guide

## Module Usage Patterns

### 1. CodeChangeTracker - Recording & Staging Changes

```cpp
#include "CodeChangeTracker.h"

// Create tracker
CodeChangeTracker tracker;

// Record a file change
FileChange change;
change.filePath = "src/main.cpp";
change.changeType = ChangeType::Modified;
change.originalContent = oldCode;
change.modifiedContent = newCode;
change.totalAdditions = 50;
change.totalDeletions = 10;

tracker.recordChange(change);

// Stage for commit
tracker.stageChange(change.filePath);

// Get diff
QString diff = tracker.getDiff("src/main.cpp");

// Create changeset
ChangeSet changeset = tracker.createChangeSet("feat: Add new feature", "main");

// Save to file
tracker.saveChangesToFile("changes.json");
```

### 2. CodeReviewOrchestrator - Multi-Agent Review

```cpp
#include "CodeReviewOrchestrator.h"

// Create orchestrator
CodeReviewOrchestrator reviewer;

// Assign reviewers by role
QStringList maintainers = {"agent-maintainer-1"};
QStringList developers = {"agent-dev-1", "agent-dev-2"};
QStringList security = {"agent-security-1"};

reviewer.assignReviewersByRole(changeset, maintainers, developers, security);

// Conduct parallel review
CodeReviewResult result = reviewer.conductParallelReview(changeset, reviewerIds);

// Check approval
if (result.canMerge && result.finalDecision == ReviewDecision::Approve) {
    // Proceed with merge
}

// Get approval summary
QString summary = reviewer.getApprovalSummary(result);
qDebug() << summary;

// Save review for audit
reviewer.saveReviewToFile(result.reviewId, "reviews/review_123.json");
```

### 3. CodeChangeValidator - Policy Enforcement

```cpp
#include "CodeChangeValidator.h"

// Create validator
CodeChangeValidator validator;

// Validate changeset
ValidationResult result = validator.validateChangeSet(changeset);

// Check for violations
if (!result.isValid) {
    qDebug() << "Validation failed:";
    for (const auto &violation : result.violations) {
        qDebug() << violation.severity << ":" << violation.message;
    }
}

// Configure policies
validator.setMaxFileSizeKb(5 * 1024);  // 5MB limit
validator.setMaxFilesPerCommit(50);
validator.setMinCommitMessageLength(20);

// Query rules
QVector<ValidationRule> securityRules = validator.getRulesByCategory("security");
```

### 4. CodeQualityAnalyzer - Metrics & Issues

```cpp
#include "CodeQualityAnalyzer.h"

// Create analyzer
CodeQualityAnalyzer analyzer;

// Analyze changeset
CodeQualityReport report = analyzer.analyzeChangeSet(changeset);

// Check quality score
if (report.overallScore < 50.0f) {
    qDebug() << "Quality issues found:";
    for (const auto &issue : report.issues) {
        if (issue.severity == "error") {
            qDebug() << "ERROR:" << issue.description;
        }
    }
}

// Track improvement
float improvement = report.scoreImprovement;
qDebug() << "Score improvement:" << improvement << "%";

// Get metrics
CodeQualityMetrics metrics = report.afterMetrics;
qDebug() << "Complexity:" << metrics.averageCyclomaticComplexity;
qDebug() << "Coverage:" << metrics.testCoverage << "%";
```

---

## Integration with Phase 1

### Using AgentScheduler for Parallel Reviews

```cpp
#include "AgentScheduler.h"
#include "CodeReviewOrchestrator.h"

// Create scheduler with Phase 2
AgentScheduler scheduler;
CodeReviewOrchestrator orchest;

// Configure schedule
ScheduleConfig config;
config.executionMode = ExecutionMode::Parallel;
config.aggregationMode = ResultAggregationMode::Majority;
config.maxConcurrentAgents = 5;

// Schedule multiple reviews
QVector<SubAgentTask> reviewTasks;
for (int i = 0; i < 3; ++i) {
    SubAgentTask task;
    task.taskId = QString("review-%1").arg(i);
    task.type = "code_review";
    reviewTasks.append(task);
}

ScheduleResult result = scheduler.scheduleMultipleTasks(reviewTasks, config);
```

### Using SubAgentSystem for Reviewer Agents

```cpp
#include "SubAgentSystem.h"

// Create sub-agent system
SubAgentSystem subAgentSystem;

// Spawn reviewer agents
QStringList reviewerIds;
for (int i = 0; i < 3; ++i) {
    QString agentId = subAgentSystem.spawnSubAgent(
        QString("reviewer-%1").arg(i),
        "code_review"
    );
    reviewerIds.append(agentId);
}

// Submit review task
SubAgentTask task;
task.taskId = "review-task-001";
task.type = "code_review";
// ... fill task parameters

SubAgentResult result = subAgentSystem.submitTask(task);
```

---

## Data Flow Example

```
1. CODE CHANGE RECORDED
   User makes changes → FileChange struct created
   → CodeChangeTracker.recordChange()
   
2. VALIDATION
   CodeChangeValidator.validateChange()
   → Returns ValidationResult with violations
   
3. QUALITY ANALYSIS
   CodeQualityAnalyzer.analyzeChangeSet()
   → Returns CodeQualityReport with metrics
   
4. MULTI-AGENT REVIEW (Phase 1 + 2)
   CodeReviewOrchestrator.assignReviewers()
   → AgentScheduler.scheduleMultipleTasks()
   → SubAgentSystem.spawnSubAgent() for each reviewer
   → Parallel review execution
   → Results aggregated via majority voting
   
5. MERGE DECISION
   if (validationPassed && qualityGood && reviewApproved) {
       merge_to_main_branch()
   }
```

---

## Common Patterns

### Pattern 1: Full Code Review Workflow

```cpp
// 1. Track changes
CodeChangeTracker tracker;
tracker.recordChange(change1);
tracker.recordChange(change2);
ChangeSet changeset = tracker.createChangeSet("Message", "branch");

// 2. Validate
CodeChangeValidator validator;
ValidationResult validation = validator.validateChangeSet(changeset);
if (!validation.isValid) return false;

// 3. Analyze quality
CodeQualityAnalyzer analyzer;
CodeQualityReport quality = analyzer.analyzeChangeSet(changeset);
if (quality.overallScore < 70.0f) return false;

// 4. Conduct review
CodeReviewOrchestrator reviewer;
reviewer.assignReviewers(changeset, reviewerList);
CodeReviewResult review = reviewer.conductParallelReview(changeset, reviewerList);

// 5. Make decision
return review.canMerge && validation.isValid && quality.overallScore > 70.0f;
```

### Pattern 2: Policy Configuration

```cpp
CodeChangeValidator validator;

// Add custom rule
ValidationRule customRule;
customRule.ruleId = "company-naming";
customRule.name = "Company Naming Standards";
customRule.description = "Files must follow CamelCase convention";
customRule.category = "naming";
customRule.priority = 8;
validator.addRule(customRule);

// Configure limits
validator.setMaxFileSizeKb(20 * 1024);    // 20MB
validator.setMaxFilesPerCommit(100);       // 100 files per commit
validator.setMinCommitMessageLength(50);   // 50 char minimum
validator.setMaxLinesPerFile(10000);       // 10K lines per file
```

### Pattern 3: Metrics Comparison

```cpp
CodeQualityAnalyzer analyzer;

// Analyze before changes
CodeQualityMetrics beforeMetrics;
// ... populate before metrics

// Analyze after changes
CodeQualityReport report = analyzer.analyzeChangeSet(changeset);
CodeQualityMetrics afterMetrics = report.afterMetrics;

// Calculate improvement
float improvement = analyzer.calculateScoreImprovement(beforeMetrics, afterMetrics);

if (improvement > 0) {
    qDebug() << "Quality improved by" << improvement << "%";
} else {
    qDebug() << "Quality degraded by" << -improvement << "%";
}
```

---

## Configuration Best Practices

### Conservative Settings (High Quality Bar)
```cpp
CodeChangeValidator validator;
validator.setMaxFileSizeKb(2 * 1024);      // 2MB files max
validator.setMaxFilesPerCommit(20);         // 20 files per commit
validator.setMinCommitMessageLength(50);    // 50+ char messages
validator.setMaxLinesPerFile(3000);         // 3K lines per file

CodeQualityAnalyzer analyzer;
analyzer.setComplexityThreshold(5.0f);      // Low complexity allowed
analyzer.setTestCoverageThreshold(0.95f);   // 95% coverage required
analyzer.setDuplicationThreshold(0.05f);    // 5% duplication max
```

### Relaxed Settings (Velocity Focus)
```cpp
CodeChangeValidator validator;
validator.setMaxFileSizeKb(50 * 1024);     // 50MB files
validator.setMaxFilesPerCommit(200);        // 200 files per commit
validator.setMinCommitMessageLength(10);    // 10+ char messages
validator.setMaxLinesPerFile(15000);        // 15K lines per file

CodeQualityAnalyzer analyzer;
analyzer.setComplexityThreshold(15.0f);     // Higher complexity OK
analyzer.setTestCoverageThreshold(0.70f);   // 70% coverage OK
analyzer.setDuplicationThreshold(0.15f);    // 15% duplication OK
```

---

## Error Handling

### Validation Errors
```cpp
ValidationResult result = validator.validateChangeSet(changeset);
if (!result.isValid) {
    // Categorize violations
    for (const auto &v : result.violations) {
        if (v.severity == "error") {
            // Critical issue - block merge
            qWarning() << "CRITICAL:" << v.message;
        } else if (v.severity == "warning") {
            // Warning - may proceed with approval
            qWarning() << "WARNING:" << v.message;
        }
    }
}
```

### Quality Degradation
```cpp
CodeQualityReport report = analyzer.analyzeChangeSet(changeset);
if (report.scoreImprovement < -10.0f) {
    // Significant quality drop
    qWarning() << "Quality dropped significantly";
    // Require extra reviews
}
```

### Review Consensus Issues
```cpp
CodeReviewResult review = reviewer.conductParallelReview(changeset, reviewerIds);
if (review.consensusScore < 0.6f) {
    // Low agreement among reviewers
    qWarning() << "Reviewers disagreed - consensus only" << review.consensusScore;
    // Flag for manual review
}
```

---

## Performance Tips

1. **Batch Operations**
   ```cpp
   // Efficient: batch record
   tracker.recordBatch(manyChanges);
   
   // Less efficient: individual records
   for (auto &change : manyChanges) {
       tracker.recordChange(change);
   }
   ```

2. **Parallel Analysis**
   ```cpp
   // Use AgentScheduler for parallel analysis
   // Instead of sequential file analysis
   analyzer.analyzeChangeSet(changeset);  // Parallel internally
   ```

3. **Cache Metrics**
   ```cpp
   // Store quality metrics for trend analysis
   // Instead of recalculating each time
   ```

---

## Debugging

### Enable Verbose Logging
```cpp
// In main.cpp
qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss.zzz} %{type} %{appname} %{function}:%{line}: %{message}");

// Run with logging
CodeChangeTracker tracker;
tracker.recordChange(change);
// Check Qt debug output
```

### Inspect Serialized Data
```cpp
CodeQualityReport report = analyzer.analyzeChangeSet(changeset);
QString json = report.toJson();
qDebug() << json;  // Pretty print JSON

// Or save to file for inspection
QFile file("debug_report.json");
file.open(QIODevice::WriteOnly);
file.write(json.toUtf8());
file.close();
```

---

## Phase 1 + Phase 2 Integration Checklist

- ✅ Import Phase 1 headers (AgentScheduler, SubAgentSystem, etc.)
- ✅ Use AgentScheduler for parallel reviews
- ✅ Use SubAgentSystem to spawn reviewer agents
- ✅ Use AgentCoordinator for orchestration strategy
- ✅ Pass SubAgentTask results to Phase 2 modules
- ✅ Aggregate decisions via Phase 1 consensus mechanisms
- ✅ Integrate persistence with Phase 1 job management

---

**Quick Links**:
- Phase 1 Reference: [PHASE1_QUICK_REFERENCE.md](PHASE1_QUICK_REFERENCE.md)
- Full Delivery Report: [PHASE2_DELIVERY_REPORT.md](PHASE2_DELIVERY_REPORT.md)
- Implementation: [src/agent/](src/agent/)
