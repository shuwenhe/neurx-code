#include "tools/NeurxSkillCreatorTool.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QDebug>

NeurxSkillCreatorTool::NeurxSkillCreatorTool(ClaudeSkillManager *manager, const QString &workspaceRoot, QObject *parent)
    : BaseTool(parent)
    , m_manager(manager)
    , m_workspaceRoot(workspaceRoot)
{
}

QJsonObject NeurxSkillCreatorTool::parametersSchema() const
{
    return QJsonDocument::fromJson(R"JSON({
        "type": "object",
        "properties": {
            "skill_name": {
                "type": "string",
                "description": "Name of the skill in kebab-case (e.g., 'python-expert')"
            },
            "description": {
                "type": "string",
                "description": "Short trigger description for the skill. If omitted, a placeholder is generated."
            },
            "path": {
                "type": "string",
                "description": "Base path where the skill should be created (relative to workspace root, default: 'skills')"
            },
            "category": {
                "type": "string",
                "description": "Optional skill category, such as coding, analysis, integration, or custom"
            },
            "version": {
                "type": "string",
                "description": "Optional semantic version for the skill scaffold",
                "default": "0.1.0"
            },
            "author": {
                "type": "string",
                "description": "Optional author name to place in the skill frontmatter"
            },
            "platforms": {
                "type": "array",
                "items": { "type": "string" },
                "description": "Optional supported platforms, such as macos, linux, windows, or any"
            },
            "include_evals": {
                "type": "boolean",
                "description": "Create an evals/evals.json skeleton alongside the skill",
                "default": false
            },
            "overwrite": {
                "type": "boolean",
                "description": "Overwrite an existing skill directory if it already exists",
                "default": false
            }
        },
        "required": ["skill_name"]
    })JSON").object();
}

ToolResult NeurxSkillCreatorTool::execute(const QString &callId, const QJsonObject &args)
{
    QString skillName = args.value("skill_name").toString();
    QString relPath = args.value("path").toString("skills");
    QString description = args.value("description").toString();
    QString category = args.value("category").toString("custom");
    QString version = args.value("version").toString("0.1.0");
    QString author = args.value("author").toString();
    bool includeEvals = args.value("include_evals").toBool(false);
    bool overwrite = args.value("overwrite").toBool(false);

    if (skillName.isEmpty()) {
        return {callId, name(), true, "Error: skill_name is required"};
    }

    // Prevent path traversal in skill name
    if (skillName.contains('/') || skillName.contains('\\') || skillName.contains("..")) {
        return {callId, name(), true, "Error: Invalid skill name (cannot contain path separators)"};
    }

    QDir root(m_workspaceRoot);
    QString skillDirPath = QDir::cleanPath(root.absoluteFilePath(relPath + "/" + skillName));

    if (!skillDirPath.startsWith(QDir::cleanPath(m_workspaceRoot))) {
        return {callId, name(), true, "Error: Target path must be within the workspace root"};
    }

    QDir skillDir(skillDirPath);
    if (skillDir.exists() && !overwrite) {
        return {callId, name(), true, "Error: Skill directory already exists: " + skillDirPath};
    }

    const QString skillTitle = titleCase(skillName);
    const QString normalizedDescription = normalizeDescription(skillName, description);
    const QStringList platforms = args.contains("platforms")
        ? [&args]() {
              QStringList out;
              const auto values = args.value("platforms").toArray();
              for (const auto &value : values) {
                  const QString platform = value.toString().trimmed().toLower();
                  if (!platform.isEmpty()) {
                      out.append(platform);
                  }
              }
              return out;
          }()
        : QStringList{QStringLiteral("any")};

    if (skillDir.exists() && overwrite) {
        // Refresh the existing tree rather than leaving stale scaffold files behind.
        const QStringList filesToRemove = {
            skillDirPath + "/SKILL.md",
            skillDirPath + "/scripts/example_script.cjs",
            skillDirPath + "/references/example_reference.md",
            skillDirPath + "/assets/example_asset.txt",
            skillDirPath + "/evals/evals.json"
        };
        for (const QString &filePath : filesToRemove) {
            QFile::remove(filePath);
        }
    }

    QString skillTemplate = buildSkillMarkdown(
        skillName,
        normalizedDescription,
        version,
        category,
        platforms,
        author
    );

    const QString exampleScript = buildScriptTemplate(skillName);
    const QString referenceTemplate = buildReferenceTemplate(skillTitle);
    const QString assetTemplate = QStringLiteral("Placeholder asset for %1. Replace with supporting files as needed.\n").arg(skillTitle);

    // Create directories.
    if (!ensureDirectoryExists(skillDirPath) ||
        !ensureDirectoryExists(skillDirPath + "/scripts") ||
        !ensureDirectoryExists(skillDirPath + "/references") ||
        !ensureDirectoryExists(skillDirPath + "/assets") ||
        (includeEvals && !ensureDirectoryExists(skillDirPath + "/evals"))) {
        return {callId, name(), true, "Error: Failed to create skill directory structure"};
    }

    // Write files.
    if (!writeToFile(skillDirPath + "/SKILL.md", skillTemplate) ||
        !writeToFile(skillDirPath + "/scripts/example_script.cjs", exampleScript) ||
        !writeToFile(skillDirPath + "/references/example_reference.md", referenceTemplate) ||
        !writeToFile(skillDirPath + "/assets/example_asset.txt", assetTemplate) ||
        (includeEvals && !writeToFile(skillDirPath + "/evals/evals.json", buildEvalTemplate(skillName, skillTitle)))) {
        return {callId, name(), true, "Error: Failed to write skill files"};
    }

    if (m_manager) {
        m_manager->refresh([](int, const QString &) {});
    }

    QString message = QString("✅ Skill '%1' initialized at %2").arg(skillName, relPath + "/" + skillName);
    return {callId, name(), false, message};
}

bool NeurxSkillCreatorTool::ensureDirectoryExists(const QString &dirPath)
{
    QDir dir(dirPath);
    if (dir.exists()) return true;
    return dir.mkpath(".");
}

bool NeurxSkillCreatorTool::writeToFile(const QString &filePath, const QString &content)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << content;
    return file.commit();
}

QString NeurxSkillCreatorTool::titleCase(const QString &name) const
{
    QStringList parts = name.split('-');
    for (int i = 0; i < parts.size(); ++i) {
        if (!parts[i].isEmpty()) {
            parts[i][0] = parts[i][0].toUpper();
        }
    }
    return parts.join(' ');
}

QString NeurxSkillCreatorTool::normalizeDescription(const QString &skillName, const QString &description) const
{
    if (!description.trimmed().isEmpty()) {
        return description.trimmed();
    }

    QString readableName = skillName;
    readableName.replace('-', ' ');
    return QStringLiteral("Use this skill when the user needs %1-related help or workflows. "
                          "Be explicit about when to trigger it, what input it expects, and what output it produces.")
        .arg(readableName);
}

QString NeurxSkillCreatorTool::platformListToMarkdown(const QStringList &platforms) const
{
    if (platforms.isEmpty()) {
        return QStringLiteral("[any]");
    }

    QStringList normalized = platforms;
    for (QString &platform : normalized) {
        platform = platform.trimmed().toLower();
    }
    return QStringLiteral("[%1]").arg(normalized.join(QStringLiteral(", ")));
}

QString NeurxSkillCreatorTool::buildSkillMarkdown(const QString &skillName,
                                                  const QString &description,
                                                  const QString &version,
                                                  const QString &category,
                                                  const QStringList &platforms,
                                                  const QString &author) const
{
    const QString skillTitle = titleCase(skillName);
    QString frontmatter;
    frontmatter += QStringLiteral("---\n");
    frontmatter += QStringLiteral("name: %1\n").arg(skillName);
    frontmatter += QStringLiteral("description: %1\n").arg(description);
    frontmatter += QStringLiteral("version: %1\n").arg(version);
    frontmatter += QStringLiteral("category: %1\n").arg(category);
    frontmatter += QStringLiteral("platforms: %1\n").arg(platformListToMarkdown(platforms));
    frontmatter += QStringLiteral("tags: [neurx, claude-skill]\n");
    if (!author.trimmed().isEmpty()) {
        frontmatter += QStringLiteral("author: %1\n").arg(author.trimmed());
    }
    frontmatter += QStringLiteral("---\n\n");

    QString body;
    body += QStringLiteral("# %1\n\n").arg(skillTitle);
    body += QStringLiteral("## Overview\n\n");
    body += QStringLiteral("Write a concise overview of what this skill enables and the situations where it should be used.\n\n");
    body += QStringLiteral("## Trigger Guidance\n\n");
    body += QStringLiteral("Be explicit about the user phrases, file types, or workflows that should trigger this skill.\n\n");
    body += QStringLiteral("## Instructions\n\n");
    body += QStringLiteral("- Explain the concrete workflow the model should follow.\n");
    body += QStringLiteral("- Keep the instructions deterministic and reusable.\n");
    body += QStringLiteral("- Prefer small, repeatable steps over vague advice.\n\n");
    body += QStringLiteral("## Resources\n\n");
    body += QStringLiteral("### `scripts/`\nAutomation helpers for deterministic operations.\n\n");
    body += QStringLiteral("### `references/`\nBackground docs, examples, or checklists loaded on demand.\n\n");
    body += QStringLiteral("### `assets/`\nTemplates, boilerplate, or binary files needed by the skill.\n\n");
    body += QStringLiteral("## Notes\n\n");
    body += QStringLiteral("- Keep `SKILL.md` focused and readable.\n");
    body += QStringLiteral("- Move large reference material into `references/`.\n");
    body += QStringLiteral("- Put deterministic automation in `scripts/`.\n");
    body += QStringLiteral("- Keep the frontmatter description strong enough that the skill triggers reliably.\n");

    return frontmatter + body;
}

QString NeurxSkillCreatorTool::buildScriptTemplate(const QString &skillName) const
{
    return QStringLiteral(R"(#!/usr/bin/env node
/**
 * Example helper script for %1.
 * Replace this with deterministic automation for the skill.
 */
async function main() {
  try {
    process.stdout.write("Success: ready for %1.\n");
  } catch (err) {
    process.stderr.write(`Failure: ${err.message}\n`);
    process.exit(1);
  }
}

main();
)").arg(skillName);
}

QString NeurxSkillCreatorTool::buildReferenceTemplate(const QString &skillTitle) const
{
    return QStringLiteral("# %1 Reference\n\nAdd workflow details, examples, and edge cases here.\n").arg(skillTitle);
}

QString NeurxSkillCreatorTool::buildEvalTemplate(const QString &skillName, const QString &skillTitle) const
{
    return QStringLiteral(R"JSON({
  "skill_name": "%1",
  "evals": [
    {
      "id": 1,
      "prompt": "Create a simple, realistic prompt for %2 and verify the skill triggers on it.",
      "expected_output": "A correct skill draft or scaffold relevant to the prompt.",
      "files": []
    }
  ]
}
)JSON").arg(skillName, skillTitle);
}
