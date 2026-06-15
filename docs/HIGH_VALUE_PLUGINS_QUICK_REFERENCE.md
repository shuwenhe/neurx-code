# NeurX-Code 高价值插件快速参考

**生成日期**: 2026-06-12

---

## 📋 6 个高价值插件总览

```
NeurX-Code 高价值插件集
├── 1. CodeReviewEngine           [PR 自动审查 - 5 Agent 并行]
├── 2. CommitCommandManager       [Git 一键工作流]
├── 3. FeatureDevelopmentWorkflow [7 阶段功能开发]
├── 4. SecurityAnalyzer           [三层安全防护]
├── 5. HookifyManager             [自定义 Hook 生成器]
└── 6. PRReviewAgents             [多维度 PR 审查]
```

---

## 🎯 快速命令参考

### 1️⃣ CodeReviewEngine - 代码审查

**命令**: `/code-review`

```cpp
// 使用方式
CodeReviewEngine engine;
CodeReviewEngine::ReviewContext ctx {
    "PR-123",                    // PR 号
    "feature/xyz",               // 分支
    "main",                      // 目标分支
    {"src/main.cpp", "src/ui"}, // 改变的文件
    "alice@example.com",         // 作者
    "Add new API",               // 描述
    {"api", "backend"}           // 标签
};

// 执行完整审查
auto result = engine.reviewPullRequest(ctx, CodeReviewEngine::FullReview);

// 检查结果
if (result.overallScore >= result.passedScore) {
    qDebug() << "✅ PR 通过审查";
}
```

**审查维度**:
- 🐛 Bug 检测
- 📚 最佳实践
- ⚡ 性能分析
- 🔒 安全审查
- 📖 文档检查

---

### 2️⃣ CommitCommandManager - Git 工作流

**命令**: `/commit`, `/commit-push-pr`, `/clean_gone`

```cpp
// 使用方式
CommitCommandManager git;
git.setCommitStyle("conventional");

// 1️⃣ 只提交
git.executeCommit("/path/to/repo");
// 输出: ✅ Committed with message

// 2️⃣ 提交→推送→PR
git.executeCommitPushPR("/path/to/repo");
// 输出: ✅ PR created: https://github.com/.../pull/456

// 3️⃣ 清理已删除分支
git.executeCleanGone("/path/to/repo");
// 输出: ✅ Deleted 5 gone branches
```

**提交风格**:
- `conventional` - Conventional Commits (默认)
- `descriptive` - 描述性格式
- `semantic` - 语义化格式

**信号**:
```cpp
connect(&git, &CommitCommandManager::prCreated,
        this, [](const QString &url) {
    qDebug() << "PR 创建:" << url;
});
```

---

### 3️⃣ FeatureDevelopmentWorkflow - 功能开发

**命令**: `/feature-dev`

```cpp
// 使用方式
FeatureDevelopmentWorkflow workflow;

// 创建功能规范
FeatureDevelopmentWorkflow::FeatureSpec spec {
    "FEAT-001",                          // 功能 ID
    "User Authentication",               // 功能名
    "Implement OAuth2 authentication",   // 描述
    "bob@example.com",                   // 所有者
    "auth-oauth2",                       // 分支
    "main",                              // 目标分支
    {"Users can login via OAuth2"},      // 接受标准
    {},                                  // 依赖
    8,                                   // 故事点
    "high",                              // 优先级
    1700000000                           // 截止时间
};

// 启动工作流
auto state = workflow.startFeatureDevelopment(spec);
// 当前阶段: Discovery (0%)

// 推进到下一阶段
workflow.advancePhase("FEAT-001");
// 推进到: Design (14%)

// 生成部署计划
auto plan = workflow.generateDeploymentPlan("FEAT-001");
// 包含: 回滚流程、监控计划、测试
```

**7 个阶段**:
1. 📋 Discovery (需求收集)
2. 🏗️ Design (架构设计)
3. 💻 Implementation (代码实现)
4. ✅ Testing (单元测试)
5. 🔗 Integration (系统集成)
6. 👀 Review (代码审查)
7. 🚀 Deployment (发布部署)

**质量检查**:
```cpp
if (workflow.checkQualityGate("FEAT-001", Phase::Implementation)) {
    qDebug() << "✅ 质量门控通过";
} else {
    auto blockers = workflow.getBlockingIssues("FEAT-001");
    qDebug() << "❌ 阻断问题数:" << blockers.count();
}
```

---

### 4️⃣ SecurityAnalyzer - 安全扫描

**命令**: `/security`

```cpp
// 使用方式
SecurityAnalyzer security;

// 扫描文件
auto result = security.scanFiles({"src/"}, true);

// 按严重程度处理
for (const auto &finding : result.findings) {
    if (finding.severity == SecurityAnalyzer::Critical) {
        qWarning() << "🔴 严重:" << finding.description;
    } else if (finding.severity == SecurityAnalyzer::High) {
        qWarning() << "🟠 高:" << finding.description;
    }
}

// 输出总结
qDebug() << "📊 扫描结果:";
qDebug() << "   总发现:" << result.totalFindings;
qDebug() << "   严重:" << result.criticalCount;
qDebug() << "   高:" << result.highCount;
qDebug() << "   风险评分:" << result.overallRiskScore << "/100";
```

**扫描类型**:
- 🔴 SQL Injection
- 🔴 XSS (Cross-Site Scripting)
- 🔴 Command Injection
- 🔴 Path Traversal
- 🔴 Cryptographic Weakness
- 🔴 Hardcoded Credentials (6 更多...)

**三层防护**:
```
Layer 1: 模式匹配 (快速)
   ↓
Layer 2: LLM 分析 (准确)
   ↓
Layer 3: Agent 验证 (可靠)
```

---

### 5️⃣ HookifyManager - Hook 管理

**命令**: `/hookify`

```cpp
// 使用方式
HookifyManager hookify;

// 分析对话找出问题模式
QStringList messages = {"...", "..."};
auto patterns = hookify.analyzeConversation(messages);

// 自动生成 Hook
for (const auto &pattern : patterns) {
    auto hook = hookify.generateHookFromPattern(pattern);
    hookify.createHook(hook);
    qDebug() << "✅ Hook 已创建:" << hook.name;
}

// 手动创建 Prevention Hook
HookifyManager::HookRule rule {
    "",                                    // 自动生成 ID
    "prevent-harmful-output",
    "Prevent harmful content",
    HookifyManager::PreventatativeHook,
    HookifyManager::Error,
    {"harmful", "dangerous", "illegal"},   // 模式
    {"damage", "hurt", "violence"},        // 关键字
    "BLOCK",                               // 动作
    "This content violates safety policy",
    true,                                  // 启用
    10                                     // 优先级
};

hookify.createHook(rule);

// 应用 Hook 到内容
QString content = "potentially problematic content";
QString filtered = hookify.applyHooks(content);
```

**Hook 类型**:
- 🛡️ Preventative - 预防不良行为
- ✏️ Correction - 实时修正
- 🔍 Filtering - 过滤输出
- 🔄 Redirection - 重定向行为
- 📋 Enforcement - 强制规则

**使用场景**:
```cpp
// 创建修正 Hook: 自动修正错别字
HookifyManager::HookRule correctionHook {
    {}, "fix-typos", "Auto-fix common typos",
    HookifyManager::CorrectionHook,
    HookifyManager::Info,
    {"recieve", "seperate", "neccessary"},
    {},
    "AUTO_REPLACE",
    "Fixed common typo",
    true, 5
};

// 创建过滤 Hook: 去除敏感信息
HookifyManager::HookRule filteringHook {
    {}, "remove-sensitive", "Remove API keys",
    HookifyManager::FilteringHook,
    HookifyManager::Warning,
    {"api.key", "api_key", "apikey"},
    {},
    "REDACT",
    "Removed sensitive data",
    true, 8
};
```

---

### 6️⃣ PRReviewAgents - PR 审查

**命令**: `/pr-review`

```cpp
// 使用方式
PRReviewAgents pr;

// 启动 PR 审查
pr.startPRReview("PR-456", "Fix critical login bug");

// 添加代码变更
PRReviewAgents::CodeDiff diff {
    "src/auth.cpp",
    "old code...",
    "new code...",
    20,          // 新增行数
    5,           // 删除行数
    "modify"
};
pr.addCodeDiff(diff);

// 执行审查
auto report = pr.reviewPR("PR-456");

// 查看结果
qDebug() << "📊 PR 审查报告:";
qDebug() << "   质量评分:" << report.overallQualityScore << "/100";
qDebug() << "   总发现数:" << report.totalFindings;
qDebug() << "   阻断项:" << report.blockers.count();
qDebug() << "   建议:" << report.suggestions.count();
qDebug() << "   可合并:" << (report.readyToMerge ? "✅ 是" : "❌ 否");

// 评估合并就绪
auto readiness = pr.assessMergeReadiness("PR-456");
qDebug() << "   合并推荐评分:" << readiness.recommendedMergeScore;
```

**7 维度审查**:

| 维度 | Agent | 检查项 |
|------|--------|--------|
| 🔒 | SecurityReviewer | 漏洞、权限、加密 |
| ⚡ | PerformanceReviewer | 效率、内存、并发 |
| 🏗️ | ArchitectureReviewer | 架构、设计模式、耦合 |
| 📖 | DocumentationReviewer | 文档、注释、示例 |
| ✅ | TestReviewer | 测试覆盖率、测试质量 |
| 💎 | CodeQualityReviewer | 风格、复杂度、可读性 |
| 🔌 | APIReviewer | API 设计、向后兼容 |

**信号**:
```cpp
connect(&pr, QOverload<const QString&>::of(&PRReviewAgents::reviewCompleted),
        this, [](const QString& prId) {
    qDebug() << "✅ PR 审查完成:" << prId;
});
```

---

## 📊 性能预期

| 操作 | 平均时间 | 备注 |
|------|---------|------|
| PR 审查 (5 agent) | 3-5 秒 | 并行执行 |
| 一键提交-推送-PR | 1-2 秒 | 依赖网络 |
| 启动功能工作流 | 100ms | 创建阶段 |
| 安全扫描 | 5-10 秒 | 取决于代码量 |
| Hook 分析 | 200-500ms | 对话长度 |
| 多维 PR 审查 | 2-3 秒 | 7 个 agent |

---

## ✅ 集成检查清单

确保这些系统正确集成:

- [x] SlashCommandManager 路由命令
- [x] AgentEngine 执行任务
- [x] EventBus 发送事件
- [x] MCPManager 调用外部工具
- [x] LLMProvider 调用 Claude API
- [x] ToolRegistry 注册工具
- [x] 日志系统记录操作
- [x] 配置系统管理设置

---

## 🚀 常见使用场景

### 场景 1: 完整 PR 工作流
```
1. 开发完成 → /commit-push-pr
2. PR 创建后 → /pr-review
3. 获得 7 维度审查
4. 修复问题
5. 再次 PR 审查
6. 合并代码
```

### 场景 2: 功能从规划到发布
```
1. /feature-dev 启动 7 阶段流程
2. 每个阶段通过质量门控
3. 完成后自动生成部署计划
4. /code-review 审查实现
5. /security 安全扫描
6. 部署上线
```

### 场景 3: 安全和代码质量保证
```
1. 代码提交 → /security 扫描
2. 自动检测 OWASP Top 10
3. 三层防护: 模式 → LLM → Agent
4. /code-review 检查代码质量
5. 生成审查报告
```

### 场景 4: 团队协作规范
```
1. 分析团队对话找问题模式
2. /hookify 自动生成 Hook
3. 防止重复问题
4. 自动修正常见错误
5. 保证代码标准
```

---

## 📚 相关文档

- 📖 [完整实现指南](HIGH_VALUE_PLUGINS_IMPLEMENTATION.md)
- 📊 [验证报告](HIGH_VALUE_PLUGINS_VERIFICATION_REPORT.md)
- 📋 [功能清单](CORE_FEATURES_INVENTORY.md)
- ✅ [对标报告](COMPLETE_FEATURE_PARITY_REPORT.md)

---

## 🔧 配置示例

### 代码审查配置
```cpp
CodeReviewEngine engine;
engine.setPassingScore(80.0);        // 通过分数
engine.setMaxConcurrentReviews(5);   // 最大并发
engine.setReviewTimeoutMs(300000);   // 超时 5 分钟
```

### 提交命令配置
```cpp
CommitCommandManager git;
git.setCommitStyle("conventional");  // Conventional Commits
// 支持: conventional, descriptive, semantic
```

### 安全扫描配置
```cpp
SecurityAnalyzer security;
security.setSeverityThreshold(SecurityAnalyzer::Medium);
security.setDeepScan(true);
security.includeDependencies(true);
```

### 功能工作流配置
```cpp
FeatureDevelopmentWorkflow workflow;
workflow.setAutoAdvance(false);      // 手动推进
workflow.setQualityGateStrict(true); // 严格检查
```

---

## 💡 最佳实践

### 1. 代码审查
- ✓ 每个 PR 都执行完整审查
- ✓ 检查 信心评分 > 80%
- ✓ 阻止严重问题合并
- ✗ 忽略低信心警告

### 2. Git 工作流
- ✓ 使用 `conventional` 提交风格
- ✓ 一键工作流自动化流程
- ✓ 定期清理已删除分支
- ✗ 手动提交容易出错

### 3. 功能开发
- ✓ 所有功能使用 7 阶段流程
- ✓ 通过每个质量门控
- ✓ 生成部署计划
- ✗ 跳过测试和审查阶段

### 4. 安全检查
- ✓ 新代码必须过安全扫描
- ✓ 修复所有严重/高风险
- ✓ 定期依赖扫描
- ✗ 忽视安全警告

### 5. Hook 管理
- ✓ 定期分析团队对话
- ✓ 自动生成常见问题 Hook
- ✓ 优先级合理设置
- ✗ 创建过多无关 Hook

### 6. PR 审查
- ✓ 启用所有 7 种审查
- ✓ 检查合并就绪评分
- ✓ 处理所有阻断项
- ✗ 忽视某个维度的审查

---

**快速参考指南**  
**生成**: 2026-06-12  
**状态**: ✅ 完成