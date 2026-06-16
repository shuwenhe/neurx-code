#include "tools/FileSystemTool.h"
#include "tools/CheckpointManager.h"
#include "services/FileService.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonDocument>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QFileDevice>
#include <QDirIterator>
#include <QRegularExpression>
#include <functional>
#include <climits>

namespace {

bool isPathInsideWorkspace(const QString &path, const QString &workspaceRoot)
{
    const QString cleanRoot = QDir::cleanPath(workspaceRoot);
    const QString cleanPath = QDir::cleanPath(path);
    if (cleanRoot.isEmpty() || cleanPath.isEmpty())
        return false;

    if (cleanPath == cleanRoot)
        return true;

    const QString relative = QDir(cleanRoot).relativeFilePath(cleanPath);
    return !relative.isEmpty()
        && !relative.startsWith(QStringLiteral(".."))
        && !QDir::isAbsolutePath(relative);
}

bool writeFileAtomically(const QString &path, const QString &content, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << content;
    out.flush();
    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

bool writeBytesAtomically(const QString &path, const QByteArray &content, QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = file.errorString();
        return false;
    }

    if (file.write(content) != content.size()) {
        if (error)
            *error = file.errorString();
        return false;
    }

    if (!file.commit()) {
        if (error)
            *error = file.errorString();
        return false;
    }
    return true;
}

QStringList splitLinesKeepSimple(const QString &text)
{
    return text.split(QRegularExpression(QStringLiteral("\r\n|\n|\r")), Qt::KeepEmptyParts);
}

QByteArray readFileBytes(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return file.readAll();
}

QString readTextRange(const QString &content, int startLine, int endLine)
{
    if (startLine <= 1 && endLine == INT_MAX)
        return content;

    const QStringList lines = splitLinesKeepSimple(content);
    const int begin = qMax(1, startLine);
    const int finish = qMin(endLine, lines.size());
    if (begin > finish)
        return {};

    QString result;
    for (int i = begin - 1; i < finish; ++i) {
        result += lines.at(i);
        if (i + 1 < finish)
            result += QLatin1Char('\n');
    }
    return result;
}

bool replaceTextBlock(QString &text, const QString &needle, const QString &replacement, bool useRegex, bool caseSensitive, int *replacementCount, QString *error)
{
    if (needle.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("Search text cannot be empty.");
        return false;
    }

    int count = 0;
    if (useRegex) {
        QRegularExpression::PatternOptions options;
        if (!caseSensitive)
            options |= QRegularExpression::CaseInsensitiveOption;
        options |= QRegularExpression::UseUnicodePropertiesOption;
        QRegularExpression rx(needle, options);
        if (!rx.isValid()) {
            if (error)
                *error = QStringLiteral("Invalid regex: %1").arg(rx.errorString());
            return false;
        }
        QRegularExpressionMatchIterator it = rx.globalMatch(text);
        while (it.hasNext()) {
            it.next();
            ++count;
        }
        text.replace(rx, replacement);
    } else {
        const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        int index = 0;
        while ((index = text.indexOf(needle, index, cs)) != -1) {
            ++count;
            text.replace(index, needle.size(), replacement);
            index += replacement.size();
        }
    }

    if (replacementCount)
        *replacementCount = count;
    return true;
}

bool copyDirectoryRecursive(const QString &source, const QString &destination)
{
    QFileInfo srcInfo(source);
    if (!srcInfo.exists())
        return false;

    if (srcInfo.isDir()) {
        QDir destDir(destination);
        if (!destDir.exists() && !QDir().mkpath(destination))
            return false;

        QDir sourceDir(source);
        const QFileInfoList entries = sourceDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            const QString targetPath = QDir(destination).filePath(entry.fileName());
            if (entry.isDir()) {
                if (!copyDirectoryRecursive(entry.absoluteFilePath(), targetPath))
                    return false;
            } else {
                QDir().mkpath(QFileInfo(targetPath).absolutePath());
                if (QFile::exists(targetPath))
                    QFile::remove(targetPath);
                if (!QFile::copy(entry.absoluteFilePath(), targetPath))
                    return false;
            }
        }
        return true;
    }

    QDir().mkpath(QFileInfo(destination).absolutePath());
    if (QFile::exists(destination))
        QFile::remove(destination);
    return QFile::copy(source, destination);
}

QFileDevice::Permissions permissionsFromOctal(const QString &modeString, bool *ok = nullptr)
{
    QString trimmed = modeString.trimmed();
    if (trimmed.startsWith("0"))
        trimmed.remove(0, 1);
    bool parsed = false;
    const int mode = trimmed.toInt(&parsed, 8);
    if (ok)
        *ok = parsed;
    if (!parsed)
        return {};

    QFileDevice::Permissions perms;
    if (mode & 0400) perms |= QFileDevice::ReadOwner;
    if (mode & 0200) perms |= QFileDevice::WriteOwner;
    if (mode & 0100) perms |= QFileDevice::ExeOwner;
    if (mode & 0040) perms |= QFileDevice::ReadGroup;
    if (mode & 0020) perms |= QFileDevice::WriteGroup;
    if (mode & 0010) perms |= QFileDevice::ExeGroup;
    if (mode & 0004) perms |= QFileDevice::ReadOther;
    if (mode & 0002) perms |= QFileDevice::WriteOther;
    if (mode & 0001) perms |= QFileDevice::ExeOther;
    return perms;
}

QString fileStatJson(const QFileInfo &fi)
{
    QJsonObject result;
    result["exists"] = fi.exists();
    result["path"] = fi.absoluteFilePath();
    result["canonical_path"] = fi.canonicalFilePath();
    result["file_name"] = fi.fileName();
    result["suffix"] = fi.suffix();
    result["size"] = static_cast<qint64>(fi.size());
    result["is_file"] = fi.isFile();
    result["is_directory"] = fi.isDir();
    result["is_symlink"] = fi.isSymLink();
    result["is_hidden"] = fi.isHidden();
    result["readable"] = fi.isReadable();
    result["writable"] = fi.isWritable();
    result["executable"] = fi.isExecutable();
    result["created_time_ms"] = fi.birthTime().toMSecsSinceEpoch();
    result["modified_time_ms"] = fi.lastModified().toMSecsSinceEpoch();
    result["accessed_time_ms"] = fi.lastRead().toMSecsSinceEpoch();
    result["permissions"] = static_cast<int>(fi.permissions());
    return QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact));
}

QString permissionsToString(QFileDevice::Permissions perms)
{
    QString result;
    result += (perms & QFileDevice::ReadOwner) ? "r" : "-";
    result += (perms & QFileDevice::WriteOwner) ? "w" : "-";
    result += (perms & QFileDevice::ExeOwner) ? "x" : "-";
    result += (perms & QFileDevice::ReadGroup) ? "r" : "-";
    result += (perms & QFileDevice::WriteGroup) ? "w" : "-";
    result += (perms & QFileDevice::ExeGroup) ? "x" : "-";
    result += (perms & QFileDevice::ReadOther) ? "r" : "-";
    result += (perms & QFileDevice::WriteOther) ? "w" : "-";
    result += (perms & QFileDevice::ExeOther) ? "x" : "-";
    return result;
}

QString permissionsToOctalString(QFileDevice::Permissions perms)
{
    int mode = 0;
    if (perms & QFileDevice::ReadOwner)  mode |= 0400;
    if (perms & QFileDevice::WriteOwner) mode |= 0200;
    if (perms & QFileDevice::ExeOwner)   mode |= 0100;
    if (perms & QFileDevice::ReadGroup)  mode |= 0040;
    if (perms & QFileDevice::WriteGroup) mode |= 0020;
    if (perms & QFileDevice::ExeGroup)   mode |= 0010;
    if (perms & QFileDevice::ReadOther)  mode |= 0004;
    if (perms & QFileDevice::WriteOther) mode |= 0002;
    if (perms & QFileDevice::ExeOther)   mode |= 0001;
    return QStringLiteral("%1").arg(mode, 4, 8, QLatin1Char('0'));
}

QJsonObject permissionsJson(const QFileInfo &fi, const QString &path)
{
    QJsonObject result;
    result["path"] = path;
    result["absolute_path"] = fi.absoluteFilePath();
    result["exists"] = fi.exists();
    result["is_directory"] = fi.isDir();
    result["is_file"] = fi.isFile();
    result["readable"] = fi.isReadable();
    result["writable"] = fi.isWritable();
    result["executable"] = fi.isExecutable();
    result["permissions_string"] = permissionsToString(fi.permissions());
    result["permissions_octal"] = permissionsToOctalString(fi.permissions());
    result["permissions_mask"] = static_cast<int>(fi.permissions());
    result["owner"] = fi.owner();
    result["group"] = fi.group();
    return result;
}

} // namespace

static bool isWriteOperationName(const QString &operation)
{
    return operation == QStringLiteral("write_file")
        || operation == QStringLiteral("create_file")
        || operation == QStringLiteral("delete_file")
        || operation == QStringLiteral("move_file")
        || operation == QStringLiteral("copy_file")
        || operation == QStringLiteral("move_tree")
        || operation == QStringLiteral("copy_tree")
        || operation == QStringLiteral("create_directory")
        || operation == QStringLiteral("remove")
        || operation == QStringLiteral("append")
        || operation == QStringLiteral("replace")
        || operation == QStringLiteral("write_batch")
        || operation == QStringLiteral("chmod")
        || operation == QStringLiteral("get_permissions")
        || operation == QStringLiteral("touch")
        || operation == QStringLiteral("truncate")
        || operation == QStringLiteral("symlink");
}

FileSystemTool::FileSystemTool(const QString &workspaceRoot, QObject *parent)
    : BaseTool(parent)
    , m_root(workspaceRoot)
    , m_checkpointManager(std::make_unique<CheckpointManager>(workspaceRoot))
    , m_patchTool(std::make_unique<PatchTool>(workspaceRoot))
    , m_smartFileCreator(std::make_unique<SmartFileCreator>(workspaceRoot))
{}

QJsonObject FileSystemTool::parametersSchema() const
{
    return QJsonDocument::fromJson(R"({
        "type": "object",
        "properties": {
            "operation": {
                "type": "string",
                "enum": ["read_file","write_file","append","replace","replace_batch","read_many","read_range","tail","write_batch","list_directory","create_file","delete_file","move_file","copy_file","rename","exists","find_files","search_text","get_metadata","stat_file","hash_file","get_permissions","chmod","symlink","touch","truncate","create_directory","remove","canonicalize","join","parent","diff","tree","copy_tree","move_tree","preview_patch","apply_patch","watch","unwatch"],
                "description": "The file operation to perform."
            },
            "path":    { "type": "string", "description": "Relative path from workspace root." },
            "content": { "type": "string", "description": "File content (for write/create)." },
            "search": { "type": "string", "description": "Search text for replace." },
            "replacement": { "type": "string", "description": "Replacement text for replace." },
            "other_path": { "type": "string", "description": "Second path for diff." },
            "patch": { "type": "string", "description": "Unified diff text for preview_patch/apply_patch." },
            "dry_run": { "type": "boolean", "description": "Preview changes without writing (for replace_batch)." },
            "offset": { "type": "integer", "description": "Byte offset for read_range." },
            "length": { "type": "integer", "description": "Byte length for read_range or number of lines for tail/truncate." },
            "algorithm": { "type": "string", "enum": ["sha256","sha1","md5","sha512"], "description": "Hash algorithm for hash_file.", "default": "sha256" },
            "base_dir": { "type": "string", "description": "Base directory for find/search operations." },
            "glob": { "type": "string", "description": "Glob pattern for find/search file selection.", "default": "**/*" },
            "pattern": { "type": "string", "description": "Search pattern for search_text or file name query for find_files." },
            "regex": { "type": "boolean", "description": "Treat pattern as regular expression for search_text." },
            "case_sensitive": { "type": "boolean", "description": "Case-sensitive matching for search_text." },
            "context_lines": { "type": "integer", "description": "Context lines around each match for search_text." },
            "max_results": { "type": "integer", "description": "Maximum results for find/search operations." },
            "mode": { "type": "string", "enum": ["simple","smart","template","batch","structure"], "description": "Enhanced create_file mode." },
            "intent": { "type": "string", "description": "What the new file should contain (smart mode)." },
            "template": { "type": "string", "description": "Template name for create_file." },
            "template_vars": { "type": "object", "description": "Variables for the selected template." },
            "related_files": { "type": "array", "items": { "type": "string" }, "description": "Related workspace files to guide smart creation." },
            "overwrite": { "type": "boolean", "description": "Allow replacing an existing file during create_file." },
            "create_dirs": { "type": "boolean", "description": "Create missing parent directories during create_file." },
            "structure_intent": { "type": "string", "description": "High-level intent for a created file structure." },
            "generate_missing": { "type": "boolean", "description": "Whether create_file structure mode may add inferred files." },
            "destination": { "type": "string", "description": "Destination path (for move/copy)." },
            "new_name": { "type": "string", "description": "New file name for rename." },
            "target": { "type": "string", "description": "Target path for symlink." },
            "linkPath": { "type": "string", "description": "Symlink path to create." },
            "recursive": { "type": "boolean", "description": "Perform operation recursively (for remove, copy)." },
            "force": { "type": "boolean", "description": "Force delete even if path doesn't exist (for remove)." },
            "start_line": { "type": "integer", "description": "First line to read (1-based, inclusive)." },
            "end_line":   { "type": "integer", "description": "Last line to read (1-based, inclusive)." },
            "permissions": { "type": "string", "description": "Octal permissions string for chmod, e.g. 644 or 0755." },
            "paths":   { "type": "array", "items": { "type": "string" }, "description": "Multiple paths to join (for join operation or batch reads)." },
            "files":   { "type": "array", "items": { "type": "object" }, "description": "Batch file operations (for write_batch or replace_batch)." },
            "max_depth": { "type": "integer", "description": "Maximum directory depth for tree." },
            "max_entries": { "type": "integer", "description": "Maximum entries returned by tree." },
            "show_hidden": { "type": "boolean", "description": "Include hidden files in tree." },
            "replace_regex": { "type": "boolean", "description": "Treat search as regular expression for replace." }
        },
        "required": ["operation"]
    })").object();
}

ToolResult FileSystemTool::execute(const QString &callId, const QJsonObject &args)
{
    const QString op = args["operation"].toString();
    if (m_sandboxManager && op != "create_file" && op != "write_batch" && op != "symlink") {
        const QString path = args["path"].toString();
        const QString absPath = safePath(path);
        if (absPath.isEmpty())
            return {callId, name(), true, "Path traversal denied."};
        if (m_sandboxManager->isProtectedMetadata(absPath))
            return {callId, name(), true, "Protected metadata access denied: " + path};
        const FileSystemAccessMode mode = isWriteOperationName(op)
            ? FileSystemAccessMode::Write
            : FileSystemAccessMode::Read;
        if (!m_sandboxManager->canAccess(absPath, mode))
            return {callId, name(), true, "Sandbox policy denied access: " + path};
        if (m_sandboxManager->isReadOnlyMode() && isWriteOperationName(op))
            return {callId, name(), true, "Read-only sandbox mode blocks file writes."};
    }
    if (op == "read_file")       return opReadFile(callId, args);
    if (op == "write_file")      return opWriteFile(callId, args);
    if (op == "append")          return opAppendFile(callId, args);
    if (op == "replace")         return opReplaceText(callId, args);
    if (op == "replace_batch")   return opReplaceBatch(callId, args);
    if (op == "read_many")       return opReadMany(callId, args);
    if (op == "list_directory")  return opListDir(callId, args);
    if (op == "create_file")     return opCreateFile(callId, args);
    if (op == "delete_file")     return opDeleteFile(callId, args);
    if (op == "move_file")       return opMoveFile(callId, args);
    if (op == "rename")          return opRenameFile(callId, args);
    if (op == "copy_file")       return opCopyFile(callId, args);
    if (op == "get_metadata")    return opGetMetadata(callId, args);
    if (op == "stat_file")       return opStatFile(callId, args);
    if (op == "hash_file")       return opHashFile(callId, args);
    if (op == "get_permissions") return opGetPermissions(callId, args);
    if (op == "chmod")           return opChmodFile(callId, args);
    if (op == "symlink")         return opSymlinkFile(callId, args);
    if (op == "touch")           return opTouchFile(callId, args);
    if (op == "truncate")        return opTruncateFile(callId, args);
    if (op == "read_range")      return opReadRangeFile(callId, args);
    if (op == "tail")            return opTailFile(callId, args);
    if (op == "write_batch")     return opWriteBatch(callId, args);
    if (op == "exists")          return opExists(callId, args);
    if (op == "find_files")      return opFindFiles(callId, args);
    if (op == "search_text")     return opSearchText(callId, args);
    if (op == "create_directory") return opCreateDirectory(callId, args);
    if (op == "remove")          return opRemove(callId, args);
    if (op == "canonicalize")    return opCanonicalize(callId, args);
    if (op == "join")            return opJoin(callId, args);
    if (op == "parent")          return opParent(callId, args);
    if (op == "diff")            return opDiffFiles(callId, args);
    if (op == "tree")            return opTree(callId, args);
    if (op == "copy_tree")       return opCopyTree(callId, args);
    if (op == "move_tree")       return opMoveTree(callId, args);
    if (op == "preview_patch")   return opPreviewPatch(callId, args);
    if (op == "apply_patch")     return opApplyPatch(callId, args);
    if (op == "watch")           return opWatchFile(callId, args);
    if (op == "unwatch")         return opUnwatchFile(callId, args);

    return {callId, name(), true, "Unknown operation: " + op};
}

QString FileSystemTool::summary(const QJsonObject &args) const
{
    const QString op = args["operation"].toString();
    const QString path = args["path"].toString();
    const QString dest = args["destination"].toString();
    if (!dest.isEmpty())
        return op + " " + path + " -> " + dest;
    if (op == "symlink")
        return op + " " + args["linkPath"].toString() + " -> " + args["target"].toString();
    if (op == "write_batch")
        return op + QString(" (%1 files)").arg(args["files"].toArray().size());
    if (op == "replace")
        return op + " " + path + " / " + args["search"].toString();
    if (op == "replace_batch")
        return op + QString(" (%1 files)").arg(args["files"].toArray().size());
    if (op == "read_many")
        return op + QString(" (%1 files)").arg(args["paths"].toArray().size());
    if (op == "diff")
        return op + " " + path + " <-> " + args["other_path"].toString();
    if (op == "tree")
        return op + " " + path;
    if (op == "copy_tree" || op == "move_tree")
        return op + " " + path + " -> " + args["destination"].toString();
    if (op == "preview_patch" || op == "apply_patch")
        return op + QString(" (%1 chars)").arg(args["patch"].toString().size());
    if (op == "watch" || op == "unwatch")
        return op + " " + path;
    if (op == "find_files" || op == "search_text")
        return op + " " + args["pattern"].toString();
    return op + " " + path;
}

bool FileSystemTool::isWriteOperation(const QString &operation) const
{
    return isWriteOperationName(operation);
}

void FileSystemTool::setSandboxManager(SandboxManager *manager)
{
    m_sandboxManager = manager;
    if (m_patchTool)
        m_patchTool->setSandboxManager(manager);
    if (m_smartFileCreator)
        m_smartFileCreator->setSandboxManager(manager);
}

QString FileSystemTool::safePath(const QString &rel) const
{
    if (rel.trimmed().isEmpty())
        return {};
    const QFileInfo info(rel);
    if (info.isAbsolute())
        return QDir::cleanPath(info.absoluteFilePath());
    return QDir::cleanPath(m_root.absoluteFilePath(rel));
}

QString FileSystemTool::workspaceRelativePath(const QString &relOrAbsPath) const
{
    const QString abs = safePath(relOrAbsPath);
    if (abs.isEmpty())
        return {};
    return m_root.relativeFilePath(abs);
}

QString FileSystemTool::checkpointPaths(const QStringList &paths, const QString &description) const
{
    if (!m_checkpointManager || !m_checkpointManager->isAvailable())
        return {};

    QStringList relPaths;
    for (const QString &path : paths) {
        const QString rel = workspaceRelativePath(path);
        if (!rel.isEmpty() && QFileInfo::exists(m_root.absoluteFilePath(rel)) && !relPaths.contains(rel))
            relPaths.append(rel);
    }

    if (relPaths.isEmpty())
        return {};

    return m_checkpointManager->checkpoint(relPaths, description);
}

ToolResult FileSystemTool::opReadFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {callId, name(), true, "Cannot open: " + f.errorString()};

    QTextStream in(&f);
    const int startLine = args.value("start_line").toInt(1);
    const int endLine   = args.value("end_line").toInt(INT_MAX);

    QString result;
    int lineNum = 0;
    while (!in.atEnd()) {
        ++lineNum;
        const QString line = in.readLine();
        if (lineNum < startLine) continue;
        if (lineNum > endLine)   break;
        result += QString::number(lineNum) + "\t" + line + "\n";
    }
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opWriteFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    const QString checkpointId = checkpointPaths({args["path"].toString()},
                                                 QStringLiteral("file_system write %1").arg(args["path"].toString()));

    QFileInfo fi(path);
    m_root.mkpath(fi.dir().absolutePath());

    QString error;
    if (!writeFileAtomically(path, args["content"].toString(), &error))
        return {callId, name(), true, "Cannot write: " + error};

    QString result = "Written: " + args["path"].toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opListDir(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QDir dir(path);
    if (!dir.exists()) return {callId, name(), true, "Directory not found."};

    QStringList entries;
    for (const auto &e : dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot,
                                           QDir::DirsFirst | QDir::Name)) {
        entries << (e.isDir() ? "[DIR]  " : "[FILE] ") + e.fileName()
                       + (e.isFile() ? QString("  (%1 B)").arg(e.size()) : "");
    }
    return {callId, name(), false, entries.join("\n")};
}

ToolResult FileSystemTool::opCreateFile(const QString &callId, const QJsonObject &args)
{
    const bool hasEnhancedArgs = args.contains("mode")
        || args.contains("intent")
        || args.contains("template")
        || args.contains("template_vars")
        || args.contains("related_files")
        || args.contains("files")
        || args.contains("structure_intent")
        || args.contains("overwrite")
        || args.contains("create_dirs")
        || args.contains("generate_missing");

    if (!hasEnhancedArgs && !args.contains("content")) {
        const QString path = safePath(args["path"].toString());
        if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
        if (QFile::exists(path)) return {callId, name(), true, "File already exists."};
        return opWriteFile(callId, args);
    }

    QJsonObject smartArgs;
    if (args.contains("mode")) smartArgs["mode"] = args.value("mode");
    if (args.contains("path")) smartArgs["path"] = args.value("path");
    if (args.contains("content")) smartArgs["content"] = args.value("content");
    if (args.contains("intent")) smartArgs["intent"] = args.value("intent");
    if (args.contains("template")) smartArgs["template"] = args.value("template");
    if (args.contains("template_vars")) smartArgs["template_vars"] = args.value("template_vars");
    if (args.contains("related_files")) smartArgs["related_files"] = args.value("related_files");
    if (args.contains("overwrite")) smartArgs["overwrite"] = args.value("overwrite");
    if (args.contains("create_dirs")) smartArgs["create_dirs"] = args.value("create_dirs");
    if (args.contains("files")) smartArgs["files"] = args.value("files");
    if (args.contains("structure_intent")) smartArgs["structure_intent"] = args.value("structure_intent");
    if (args.contains("generate_missing")) smartArgs["generate_missing"] = args.value("generate_missing");

    ToolResult result = m_smartFileCreator->execute(callId, smartArgs);
    result.name = name();
    return result;
}

ToolResult FileSystemTool::opDeleteFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    const QString checkpointId = checkpointPaths({args["path"].toString()},
                                                 QStringLiteral("file_system delete %1").arg(args["path"].toString()));

    if (!QFile::remove(path))
        return {callId, name(), true, "Failed to delete file."};
    QString result = "Deleted: " + args["path"].toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opMoveFile(const QString &callId, const QJsonObject &args)
{
    const QString src  = safePath(args["path"].toString());
    const QString dest = safePath(args["destination"].toString());
    if (src.isEmpty() || dest.isEmpty())
        return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager) {
        if (!m_sandboxManager->canAccess(src, FileSystemAccessMode::Write))
            return {callId, name(), true, "Sandbox policy denied access: " + args["path"].toString()};
        if (!m_sandboxManager->canAccess(dest, FileSystemAccessMode::Write))
            return {callId, name(), true, "Sandbox policy denied access: " + args["destination"].toString()};
    }
    const QString checkpointId = checkpointPaths(
        {args["path"].toString(), args["destination"].toString()},
        QStringLiteral("file_system move %1 -> %2")
            .arg(args["path"].toString(), args["destination"].toString()));

    if (!QFile::rename(src, dest))
        return {callId, name(), true, "Failed to move file."};
    QString result = "Moved to: " + args["destination"].toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opRenameFile(const QString &callId, const QJsonObject &args)
{
    QJsonObject moveArgs = args;
    if (!moveArgs.contains("destination")) {
        const QString src = args.value("path").toString();
        const QString newName = args.value("new_name").toString();
        if (src.isEmpty() || newName.isEmpty())
            return {callId, name(), true, "Missing destination or new_name."};
        const QString absSrc = safePath(src);
        if (absSrc.isEmpty())
            return {callId, name(), true, "Path traversal denied."};
        QFileInfo srcInfo(absSrc);
        moveArgs["destination"] = QDir(srcInfo.dir().absolutePath()).absoluteFilePath(newName);
    }
    return opMoveFile(callId, moveArgs);
}

ToolResult FileSystemTool::opCopyFile(const QString &callId, const QJsonObject &args)
{
    const QString src  = safePath(args["path"].toString());
    const QString dest = safePath(args["destination"].toString());
    if (src.isEmpty() || dest.isEmpty())
        return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager) {
        if (!m_sandboxManager->canAccess(src, FileSystemAccessMode::Read))
            return {callId, name(), true, "Sandbox policy denied access: " + args["path"].toString()};
        if (!m_sandboxManager->canAccess(dest, FileSystemAccessMode::Write))
            return {callId, name(), true, "Sandbox policy denied access: " + args["destination"].toString()};
    }
    const QString checkpointId = checkpointPaths(
        {args["path"].toString(), args["destination"].toString()},
        QStringLiteral("file_system copy %1 -> %2").arg(args["path"].toString(), args["destination"].toString()));

    // Ensure destination directory exists
    QFileInfo destInfo(dest);
    m_root.mkpath(destInfo.dir().absolutePath());

    // For files use QFile::copy; for directories perform recursive copy
    QFileInfo srcInfo(src);
    bool ok = false;
    if (srcInfo.isDir()) {
        // Recursive directory copy
        QDir sourceDir(src);
        QDir destDir(dest);
        if (!destDir.exists()) destDir.mkpath(dest);

        QFileInfoList entries = sourceDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
        ok = true;
        for (const QFileInfo &entry : entries) {
            const QString relPath = QDir(src).relativeFilePath(entry.absoluteFilePath());
            const QString targetPath = destDir.absoluteFilePath(relPath);
            if (entry.isDir()) {
                if (!QDir().mkpath(targetPath)) { ok = false; break; }
            } else {
                QDir().mkpath(QFileInfo(targetPath).dir().absolutePath());
                if (!QFile::copy(entry.absoluteFilePath(), targetPath)) { ok = false; break; }
            }
        }
    } else {
        ok = QFile::copy(src, dest);
    }

    if (!ok)
        return {callId, name(), true, "Failed to copy file/directory."};

    QString result = "Copied to: " + args["destination"].toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opGetMetadata(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QFileInfo fi(path);
    if (!fi.exists())
        return {callId, name(), true, "Path does not exist."};

    QJsonObject metadata;
    metadata["path"] = args["path"].toString();
    metadata["absolute_path"] = fi.absoluteFilePath();
    metadata["is_file"] = fi.isFile();
    metadata["is_directory"] = fi.isDir();
    metadata["is_symlink"] = fi.isSymLink();
    metadata["size"] = static_cast<qint64>(fi.size());
    metadata["created_time_ms"] = fi.birthTime().toMSecsSinceEpoch();
    metadata["modified_time_ms"] = fi.lastModified().toMSecsSinceEpoch();
    metadata["accessed_time_ms"] = fi.lastRead().toMSecsSinceEpoch();
    metadata["readable"] = fi.isReadable();
    metadata["writable"] = fi.isWritable();
    metadata["executable"] = fi.isExecutable();
    metadata["hidden"] = fi.isHidden();

    if (fi.isDir()) {
        QDir dir(path);
        metadata["entry_count"] = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).count();
    }

    return {callId, name(), false, QJsonDocument(metadata).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opCreateDirectory(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    
    const bool recursive = args.value("recursive").toBool(true);
    const QString checkpointId = checkpointPaths({args["path"].toString()},
                                                 QStringLiteral("file_system create_directory %1").arg(args["path"].toString()));

    QDir dir;
    bool ok = recursive ? dir.mkpath(path) : dir.mkdir(path);
    
    if (!ok)
        return {callId, name(), true, "Failed to create directory."};

    QString result = "Directory created: " + args["path"].toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opRemove(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    
    const bool recursive = args.value("recursive").toBool(false);
    const bool force = args.value("force").toBool(false);
    const QString checkpointId = checkpointPaths({args["path"].toString()},
                                                 QStringLiteral("file_system remove %1").arg(args["path"].toString()));

    QFileInfo fi(path);
    if (!fi.exists()) {
        if (force)
            return {callId, name(), false, "Path does not exist (force mode allows this)."};
        return {callId, name(), true, "Path does not exist."};
    }

    bool ok = false;
    if (fi.isDir()) {
        if (recursive) {
            QDir dir(path);
            ok = dir.removeRecursively();
        } else {
            ok = QDir().rmdir(path);
        }
    } else {
        ok = QFile::remove(path);
    }

    if (!ok)
        return {callId, name(), true, "Failed to remove path."};

    QString result = "Removed: " + args["path"].toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opCanonicalize(const QString &callId, const QJsonObject &args)
{
    const QString path = args["path"].toString();
    if (path.trimmed().isEmpty())
        return {callId, name(), true, "Path is empty."};

    const QFileInfo info(path);
    const QString absolute = QDir::cleanPath(info.isAbsolute() ? path : m_root.absoluteFilePath(path));
    const QString relative = m_root.relativeFilePath(absolute);

    QJsonObject result;
    result["original"] = path;
    result["absolute"] = absolute;
    result["relative"] = relative;
    result["canonical"] = QDir::cleanPath(absolute);
    result["exists"] = QFileInfo::exists(absolute);

    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opJoin(const QString &callId, const QJsonObject &args)
{
    const QJsonArray pathsArray = args["paths"].toArray();
    if (pathsArray.isEmpty())
        return {callId, name(), true, "No paths provided."};

    QStringList paths;
    for (const QJsonValue &val : pathsArray) {
        if (val.isString())
            paths << val.toString();
    }

    if (paths.isEmpty())
        return {callId, name(), true, "No valid paths provided."};

    QString result = paths.first();
    for (int i = 1; i < paths.size(); ++i) {
        result = QDir(result).filePath(paths[i]);
    }
    
    const QString canonical = QDir::cleanPath(result);
    const QString relative = m_root.relativeFilePath(canonical);

    QJsonObject response;
    response["joined"] = canonical;
    response["relative"] = relative;
    response["exists"] = QFileInfo::exists(canonical);

    return {callId, name(), false, QJsonDocument(response).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opParent(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};

    const QFileInfo fi(path);
    const QString parent = fi.dir().absolutePath();
    const QString relative = m_root.relativeFilePath(parent);

    QJsonObject result;
    result["original"] = args["path"].toString();
    result["parent"] = parent;
    result["relative"] = relative;
    result["exists"] = QFileInfo::exists(parent);

    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opStatFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QFileInfo fi(path);
    if (!fi.exists())
        return {callId, name(), true, "Path does not exist."};

    return {callId, name(), false, fileStatJson(fi)};
}

ToolResult FileSystemTool::opHashFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {callId, name(), true, "Cannot open: " + file.errorString()};

    const QString algorithm = args.value("algorithm").toString(QStringLiteral("sha256")).toLower();
    QCryptographicHash::Algorithm hashAlgo = QCryptographicHash::Sha256;
    if (algorithm == QStringLiteral("sha1")) hashAlgo = QCryptographicHash::Sha1;
    else if (algorithm == QStringLiteral("md5")) hashAlgo = QCryptographicHash::Md5;
    else if (algorithm == QStringLiteral("sha512")) hashAlgo = QCryptographicHash::Sha512;

    const QByteArray digest = QCryptographicHash::hash(file.readAll(), hashAlgo).toHex();
    QJsonObject result;
    result["path"] = args["path"].toString();
    result["algorithm"] = algorithm;
    result["hash"] = QString::fromUtf8(digest);
    result["bytes"] = static_cast<qint64>(file.size());
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opChmodFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};

    QFileInfo fi(path);
    if (!fi.exists())
        return {callId, name(), true, "Path does not exist."};

    const QString modeText = args.value("permissions").toString(args.value("mode").toString());
    bool ok = false;
    const QFileDevice::Permissions perms = permissionsFromOctal(modeText, &ok);
    if (!ok)
        return {callId, name(), true, "Invalid permissions mode: " + modeText};

    const bool recursive = args.value("recursive").toBool(false);
    QStringList changed;
    QStringList failed;

    auto applyPerms = [&](const QString &absolutePath) {
        QFile file(absolutePath);
        if (file.setPermissions(perms)) {
            changed.append(absolutePath);
        } else {
            failed.append(absolutePath);
        }
    };

    if (fi.isDir() && recursive) {
        applyPerms(path);
        QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext())
            applyPerms(it.next());
    } else {
        applyPerms(path);
    }

    if (changed.isEmpty())
        return {callId, name(), true, "Failed to change permissions."};

    QJsonObject result;
    result["path"] = args["path"].toString();
    result["permissions"] = modeText;
    result["recursive"] = recursive;
    result["changed_count"] = changed.size();
    result["failed_count"] = failed.size();
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opGetPermissions(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QFileInfo fi(path);
    if (!fi.exists())
        return {callId, name(), true, "Path does not exist."};

    return {callId, name(), false, QJsonDocument(permissionsJson(fi, args["path"].toString())).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opSymlinkFile(const QString &callId, const QJsonObject &args)
{
    const QString target = safePath(args.value("target").toString());
    const QString linkPath = safePath(args.value("linkPath").toString());
    if (target.isEmpty() || linkPath.isEmpty())
        return {callId, name(), true, "Path traversal denied."};

    const QString checkpointId = checkpointPaths(
        {args.value("target").toString(), args.value("linkPath").toString()},
        QStringLiteral("file_system symlink %1 -> %2").arg(args.value("linkPath").toString(), args.value("target").toString()));

    QFileInfo linkInfo(linkPath);
    if (linkInfo.exists()) {
        if (!QFile::remove(linkPath))
            return {callId, name(), true, "Link path already exists and could not be removed."};
    } else {
        QFileInfo parentInfo(linkPath);
        m_root.mkpath(parentInfo.dir().absolutePath());
    }

    if (!QFile::link(target, linkPath))
        return {callId, name(), true, "Failed to create symlink."};

    QString result = "Symlink created: " + args.value("linkPath").toString() + " -> " + args.value("target").toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opTouchFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};

    const QString checkpointId = checkpointPaths({args["path"].toString()},
                                                 QStringLiteral("file_system touch %1").arg(args["path"].toString()));

    QFileInfo fi(path);
    m_root.mkpath(fi.dir().absolutePath());

    QFile file(path);
    if (!file.exists()) {
        if (!file.open(QIODevice::WriteOnly))
            return {callId, name(), true, "Cannot create file: " + file.errorString()};
        file.close();
    }

    if (!file.open(QIODevice::ReadWrite))
        return {callId, name(), true, "Cannot open file: " + file.errorString()};

    file.setFileTime(QDateTime::currentDateTimeUtc(), QFileDevice::FileModificationTime);
    file.close();

    QString result = "Touched: " + args["path"].toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opTruncateFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};

    const qint64 size = static_cast<qint64>(args.value("length").toInt(0));
    if (size < 0)
        return {callId, name(), true, "Length must be non-negative."};

    const QString checkpointId = checkpointPaths({args["path"].toString()},
                                                 QStringLiteral("file_system truncate %1").arg(args["path"].toString()));

    QFileInfo fi(path);
    m_root.mkpath(fi.dir().absolutePath());

    QFile file(path);
    const QIODevice::OpenMode openMode = QFileInfo::exists(path)
        ? QIODevice::ReadWrite
        : QIODevice::WriteOnly;
    if (!file.open(openMode))
        return {callId, name(), true, "Cannot open file: " + file.errorString()};

    if (!file.resize(size))
        return {callId, name(), true, "Failed to truncate file."};

    QString result = QStringLiteral("Truncated: %1 to %2 bytes").arg(args["path"].toString()).arg(size);
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opReadRangeFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {callId, name(), true, "Cannot open: " + file.errorString()};

    const qint64 offset = static_cast<qint64>(args.value("offset").toInt(0));
    const int length = args.contains("length") ? args.value("length").toInt(-1) : -1;
    if (offset < 0)
        return {callId, name(), true, "Offset must be non-negative."};
    if (!file.seek(offset))
        return {callId, name(), true, "Failed to seek to requested offset."};

    QByteArray bytes = (length < 0) ? file.readAll() : file.read(length);
    QJsonObject result;
    result["path"] = args["path"].toString();
    result["offset"] = offset;
    result["bytes_read"] = static_cast<qint64>(bytes.size());
    result["content"] = QString::fromUtf8(bytes);
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opTailFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {callId, name(), true, "Cannot open: " + file.errorString()};

    const int lineCount = qMax(1, args.value("length").toInt(20));
    const QString text = QString::fromUtf8(file.readAll());
    const QStringList lines = text.split('\n');
    const int start = qMax(0, lines.size() - lineCount);
    QStringList tailLines;
    for (int i = start; i < lines.size(); ++i)
        tailLines.append(lines.at(i));

    QJsonObject result;
    result["path"] = args["path"].toString();
    result["line_count"] = lineCount;
    result["content"] = tailLines.join("\n");
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opWriteBatch(const QString &callId, const QJsonObject &args)
{
    const QJsonArray filesArray = args.value("files").toArray();
    if (filesArray.isEmpty())
        return {callId, name(), true, "No files provided."};

    struct BatchItem {
        QString relPath;
        QString absPath;
        QByteArray original;
        bool existed{false};
    };

    QList<BatchItem> items;
    items.reserve(filesArray.size());
    QStringList batchPaths;
    QList<QString> contents;

    for (const QJsonValue &value : filesArray) {
        const QJsonObject fileObj = value.toObject();
        const QString relPath = fileObj.value("path").toString();
        const QString absPath = safePath(relPath);
        if (relPath.trimmed().isEmpty() || absPath.isEmpty())
            return {callId, name(), true, "Invalid path in batch write."};

        if (m_sandboxManager && !m_sandboxManager->canAccess(absPath, FileSystemAccessMode::Write))
            return {callId, name(), true, "Sandbox policy denied write access: " + relPath};

        BatchItem item;
        item.relPath = relPath;
        item.absPath = absPath;
        item.existed = QFileInfo::exists(absPath);
        if (item.existed) {
            QFile existing(absPath);
            if (existing.open(QIODevice::ReadOnly))
                item.original = existing.readAll();
        }
        items.append(item);
        batchPaths.append(relPath);
        contents.append(fileObj.value("content").toString());
    }

    const QString checkpointId = checkpointPaths(batchPaths, QStringLiteral("file_system write_batch"));

    QStringList createdPaths;
    auto rollback = [&]() {
        for (const BatchItem &item : items) {
            if (item.existed) {
                writeBytesAtomically(item.absPath, item.original, nullptr);
            } else {
                QFile::remove(item.absPath);
            }
        }
    };

    for (int i = 0; i < items.size(); ++i) {
        const BatchItem &item = items.at(i);
        QFileInfo fi(item.absPath);
        m_root.mkpath(fi.dir().absolutePath());

        QString error;
        const QString content = contents.at(i);
        if (!writeFileAtomically(item.absPath, content, &error)) {
            rollback();
            return {callId, name(), true, QStringLiteral("Failed to write %1: %2").arg(item.relPath, error)};
        }
        if (!item.existed)
            createdPaths.append(item.relPath);
    }

    QJsonObject result;
    result["written"] = filesArray.size();
    result["created"] = createdPaths.size();
    result["checkpoint"] = checkpointId;
    QJsonArray writtenFiles;
    for (const BatchItem &item : items)
        writtenFiles.append(item.relPath);
    result["paths"] = writtenFiles;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opExists(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QJsonObject result;
    result["path"] = args["path"].toString();
    result["exists"] = QFileInfo::exists(path);
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opFindFiles(const QString &callId, const QJsonObject &args)
{
    const QString baseDir = safePath(args.value("base_dir").toString(args.value("path").toString(".")));
    if (baseDir.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(baseDir, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    const QString glob = args.value("glob").toString("**/*");
    const QString pattern = args.value("pattern").toString();
    const bool recursive = args.value("recursive").toBool(true);
    const int maxResults = qMax(1, args.value("max_results").toInt(200));

    const QRegularExpression wildcard = QRegularExpression::fromWildcard(
        glob,
        Qt::CaseInsensitive,
        QRegularExpression::UnanchoredWildcardConversion);
    const QRegularExpression nameRegex = pattern.isEmpty()
        ? QRegularExpression()
        : QRegularExpression::fromWildcard(pattern, Qt::CaseInsensitive, QRegularExpression::UnanchoredWildcardConversion);

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive)
        flags |= QDirIterator::Subdirectories;

    QDirIterator it(baseDir, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden, flags);
    QJsonArray matches;
    int scanned = 0;
    while (it.hasNext() && matches.size() < maxResults) {
        const QString absPath = it.next();
        ++scanned;
        const QString relPath = m_root.relativeFilePath(absPath);
        const QString fileName = QFileInfo(absPath).fileName();
        if (!wildcard.pattern().isEmpty() && !wildcard.match(relPath).hasMatch())
            continue;
        if (!pattern.isEmpty()) {
            if (nameRegex.isValid() && !nameRegex.match(fileName).hasMatch() && !nameRegex.match(relPath).hasMatch())
                continue;
        }

        QJsonObject item;
        item["path"] = relPath;
        item["absolute_path"] = absPath;
        item["size"] = static_cast<qint64>(QFileInfo(absPath).size());
        item["modified_time_ms"] = QFileInfo(absPath).lastModified().toMSecsSinceEpoch();
        matches.append(item);
    }

    QJsonObject result;
    result["base_dir"] = m_root.relativeFilePath(baseDir);
    result["glob"] = glob;
    result["pattern"] = pattern;
    result["scanned"] = scanned;
    result["returned"] = matches.size();
    result["truncated"] = matches.size() >= maxResults;
    result["matches"] = matches;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opSearchText(const QString &callId, const QJsonObject &args)
{
    const QString baseDir = safePath(args.value("base_dir").toString(args.value("path").toString(".")));
    if (baseDir.isEmpty()) return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(baseDir, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    const QString glob = args.value("glob").toString("**/*");
    const QString pattern = args.value("pattern").toString();
    const bool regexMode = args.value("regex").toBool(true);
    const bool caseSensitive = args.value("case_sensitive").toBool(false);
    const int contextLines = qMax(0, args.value("context_lines").toInt(2));
    const int maxResults = qMax(1, args.value("max_results").toInt(200));
    const bool recursive = args.value("recursive").toBool(true);

    if (pattern.trimmed().isEmpty())
        return {callId, name(), true, "Pattern cannot be empty."};

    const QRegularExpression wildcard = QRegularExpression::fromWildcard(
        glob,
        Qt::CaseInsensitive,
        QRegularExpression::UnanchoredWildcardConversion);

    QRegularExpression contentRegex;
    if (regexMode) {
        QRegularExpression::PatternOptions options;
        if (!caseSensitive)
            options |= QRegularExpression::CaseInsensitiveOption;
        options |= QRegularExpression::UseUnicodePropertiesOption;
        contentRegex = QRegularExpression(pattern, options);
        if (!contentRegex.isValid())
            return {callId, name(), true, "Invalid regex: " + contentRegex.errorString()};
    }

    QDirIterator::IteratorFlags flags = QDirIterator::NoIteratorFlags;
    if (recursive)
        flags |= QDirIterator::Subdirectories;

    QDirIterator it(baseDir, QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden, flags);
    QJsonArray matches;
    int filesSearched = 0;
    while (it.hasNext() && matches.size() < maxResults) {
        const QString absPath = it.next();
        const QString relPath = m_root.relativeFilePath(absPath);
        if (!wildcard.pattern().isEmpty() && !wildcard.match(relPath).hasMatch())
            continue;
        if (QFileInfo(absPath).size() > 10 * 1024 * 1024)
            continue;

        QFile file(absPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QByteArray peek = file.peek(2048);
        if (peek.contains('\0'))
            continue;

        QTextStream in(&file);
        int lineNumber = 0;
        QStringList lines;
        while (!in.atEnd() && matches.size() < maxResults) {
            const QString line = in.readLine();
            lines.append(line);
            ++lineNumber;
        }
        file.close();

        ++filesSearched;
        for (int i = 0; i < lines.size() && matches.size() < maxResults; ++i) {
            const QString &line = lines.at(i);
            bool matched = false;
            int column = 1;
            if (regexMode) {
                auto match = contentRegex.match(line);
                matched = match.hasMatch();
                if (matched)
                    column = match.capturedStart() + 1;
            } else {
                Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
                int idx = line.indexOf(pattern, 0, cs);
                matched = idx >= 0;
                if (matched)
                    column = idx + 1;
            }
            if (!matched)
                continue;

            QJsonObject item;
            item["path"] = relPath;
            item["line"] = i + 1;
            item["column"] = column;
            item["content"] = line;
            if (contextLines > 0) {
                QJsonArray before;
                QJsonArray after;
                for (int b = qMax(0, i - contextLines); b < i; ++b)
                    before.append(lines.at(b));
                for (int a = i + 1; a < qMin(lines.size(), i + 1 + contextLines); ++a)
                    after.append(lines.at(a));
                if (!before.isEmpty())
                    item["before_context"] = before;
                if (!after.isEmpty())
                    item["after_context"] = after;
            }
            matches.append(item);
        }
    }

    QJsonObject result;
    result["base_dir"] = m_root.relativeFilePath(baseDir);
    result["glob"] = glob;
    result["pattern"] = pattern;
    result["regex"] = regexMode;
    result["case_sensitive"] = caseSensitive;
    result["context_lines"] = contextLines;
    result["files_searched"] = filesSearched;
    result["total_matches"] = matches.size();
    result["truncated"] = matches.size() >= maxResults;
    result["matches"] = matches;
    return {callId, name(), matches.isEmpty(), QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opAppendFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty()) return {callId, name(), true, "Path traversal denied."};

    const QString checkpointId = checkpointPaths({args["path"].toString()},
                                                 QStringLiteral("file_system append %1").arg(args["path"].toString()));

    QFileInfo fi(path);
    m_root.mkpath(fi.dir().absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return {callId, name(), true, "Cannot open: " + file.errorString()};

    const QByteArray data = args.value("content").toString().toUtf8();
    if (file.write(data) != data.size())
        return {callId, name(), true, "Failed to append content."};

    QString result = "Appended: " + args["path"].toString();
    if (!checkpointId.isEmpty())
        result += "\nCheckpoint: " + checkpointId;
    return {callId, name(), false, result};
}

ToolResult FileSystemTool::opReplaceText(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty())
        return {callId, name(), true, "Path traversal denied."};

    const QString needle = args.value("search").toString();
    const QString replacement = args.value("replacement").toString();
    if (needle.trimmed().isEmpty())
        return {callId, name(), true, "Search text cannot be empty."};

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {callId, name(), true, "Cannot open: " + file.errorString()};

    const QString original = QString::fromUtf8(file.readAll());
    file.close();

    QString updated = original;
    int replacements = 0;
    const bool useRegex = args.value("replace_regex").toBool(args.value("regex").toBool(false));
    const bool caseSensitive = args.value("case_sensitive").toBool(false);

    if (useRegex) {
        QRegularExpression::PatternOptions options;
        if (!caseSensitive)
            options |= QRegularExpression::CaseInsensitiveOption;
        options |= QRegularExpression::UseUnicodePropertiesOption;
        QRegularExpression rx(needle, options);
        if (!rx.isValid())
            return {callId, name(), true, "Invalid regex: " + rx.errorString()};
        QRegularExpressionMatchIterator it = rx.globalMatch(original);
        while (it.hasNext()) {
            it.next();
            ++replacements;
        }
        updated.replace(rx, replacement);
    } else {
        const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        int index = 0;
        while ((index = updated.indexOf(needle, index, cs)) != -1) {
            ++replacements;
            updated.replace(index, needle.size(), replacement);
            index += replacement.size();
        }
    }

    if (replacements == 0) {
        QJsonObject result;
        result["path"] = args["path"].toString();
        result["replacements"] = 0;
        result["changed"] = false;
        return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
    }

    const QString checkpointId = checkpointPaths({args["path"].toString()},
                                                 QStringLiteral("file_system replace %1").arg(args["path"].toString()));

    QString error;
    if (!writeFileAtomically(path, updated, &error))
        return {callId, name(), true, "Cannot write: " + error};

    QJsonObject result;
    result["path"] = args["path"].toString();
    result["replacements"] = replacements;
    result["changed"] = true;
    if (!checkpointId.isEmpty())
        result["checkpoint"] = checkpointId;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opReadMany(const QString &callId, const QJsonObject &args)
{
    const QJsonArray pathsArray = args.value("paths").toArray();
    if (pathsArray.isEmpty())
        return {callId, name(), true, "No paths provided."};

    const int startLine = qMax(1, args.value("start_line").toInt(1));
    const int endLine = args.contains("end_line")
        ? qMax(startLine, args.value("end_line").toInt(INT_MAX))
        : INT_MAX;

    QJsonArray files;
    for (const QJsonValue &value : pathsArray) {
        const QString relPath = value.toString();
        if (relPath.trimmed().isEmpty())
            continue;

        const QString path = safePath(relPath);
        QJsonObject item;
        item["path"] = relPath;
        item["absolute_path"] = path;

        if (path.isEmpty()) {
            item["error"] = "Path traversal denied.";
            files.append(item);
            continue;
        }
        if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Read)) {
            item["error"] = "Sandbox policy denied read access.";
            files.append(item);
            continue;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            item["error"] = "Cannot open: " + file.errorString();
            files.append(item);
            continue;
        }

        const QString content = QString::fromUtf8(file.readAll());
        item["size"] = static_cast<qint64>(content.toUtf8().size());
        item["content"] = readTextRange(content, startLine, endLine);
        item["exists"] = true;
        item["truncated"] = false;
        files.append(item);
    }

    QJsonObject result;
    result["count"] = files.size();
    result["start_line"] = startLine;
    result["end_line"] = endLine == INT_MAX ? QJsonValue() : QJsonValue(endLine);
    result["files"] = files;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opPreviewPatch(const QString &callId, const QJsonObject &args)
{
    const QString patchText = args.value("patch").toString();
    if (patchText.trimmed().isEmpty())
        return {callId, name(), true, "Patch text is required."};
    if (!m_patchTool)
        return {callId, name(), true, "Patch tool is unavailable."};

    QJsonObject delegated;
    delegated["operation"] = QStringLiteral("preview_diff");
    delegated["patch"] = patchText;
    ToolResult result = m_patchTool->execute(callId, delegated);
    result.name = name();
    return result;
}

ToolResult FileSystemTool::opApplyPatch(const QString &callId, const QJsonObject &args)
{
    const QString patchText = args.value("patch").toString();
    if (patchText.trimmed().isEmpty())
        return {callId, name(), true, "Patch text is required."};
    if (!m_patchTool)
        return {callId, name(), true, "Patch tool is unavailable."};

    QJsonObject delegated;
    delegated["operation"] = QStringLiteral("apply_diff");
    delegated["patch"] = patchText;
    ToolResult result = m_patchTool->execute(callId, delegated);
    result.name = name();
    return result;
}

ToolResult FileSystemTool::opReplaceBatch(const QString &callId, const QJsonObject &args)
{
    const QJsonArray filesArray = args.value("files").toArray();
    if (filesArray.isEmpty())
        return {callId, name(), true, "No files provided."};

    const bool dryRun = args.value("dry_run").toBool(false);
    struct PlannedChange {
        QString relPath;
        QString absPath;
        QString updated;
        int replacements{0};
        QString search;
    };
    QVector<PlannedChange> planned;
    planned.reserve(filesArray.size());

    QStringList touchedPaths;
    QJsonArray preview;

    for (const QJsonValue &value : filesArray) {
        const QJsonObject item = value.toObject();
        const QString relPath = item.value("path").toString();
        const QString path = safePath(relPath);
        const QString needle = item.value("search").toString(args.value("search").toString());
        const QString replacement = item.value("replacement").toString(args.value("replacement").toString());
        const bool useRegex = item.value("replace_regex").toBool(item.value("regex").toBool(args.value("replace_regex").toBool(args.value("regex").toBool(false))));
        const bool caseSensitive = item.value("case_sensitive").toBool(args.value("case_sensitive").toBool(false));

        QJsonObject out;
        out["path"] = relPath;

        if (relPath.trimmed().isEmpty() || path.isEmpty()) {
            out["error"] = "Path traversal denied.";
            preview.append(out);
            continue;
        }
        if (m_sandboxManager && !m_sandboxManager->canAccess(path, FileSystemAccessMode::Write)) {
            out["error"] = "Sandbox policy denied write access.";
            preview.append(out);
            continue;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            out["error"] = "Cannot open: " + file.errorString();
            preview.append(out);
            continue;
        }

        QString text = QString::fromUtf8(file.readAll());
        file.close();

        int replacements = 0;
        QString error;
        if (!replaceTextBlock(text, needle, replacement, useRegex, caseSensitive, &replacements, &error)) {
            out["error"] = error;
            preview.append(out);
            continue;
        }

        out["search"] = needle;
        out["replacement"] = replacement;
        out["replacements"] = replacements;
        out["changed"] = replacements > 0;
        preview.append(out);

        if (replacements > 0) {
            planned.push_back({relPath, path, text, replacements, needle});
            if (!touchedPaths.contains(relPath))
                touchedPaths.append(relPath);
        }
    }

    if (dryRun) {
        QJsonObject result;
        result["dry_run"] = true;
        result["changed_files"] = planned.size();
        result["preview"] = preview;
        return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
    }

    if (planned.isEmpty()) {
        QJsonObject result;
        result["changed_files"] = 0;
        result["preview"] = preview;
        result["changed"] = false;
        return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
    }

    const QString checkpointId = checkpointPaths(touchedPaths, QStringLiteral("file_system replace_batch"));

    for (const PlannedChange &change : planned) {
        QString error;
        if (!writeFileAtomically(change.absPath, change.updated, &error)) {
            QJsonObject result;
            result["error"] = QStringLiteral("Cannot write %1: %2").arg(change.relPath, error);
            result["checkpoint"] = checkpointId;
            result["preview"] = preview;
            return {callId, name(), true, QJsonDocument(result).toJson(QJsonDocument::Compact)};
        }
    }

    QJsonObject result;
    result["changed_files"] = planned.size();
    result["changed"] = true;
    result["checkpoint"] = checkpointId;
    result["preview"] = preview;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opDiffFiles(const QString &callId, const QJsonObject &args)
{
    const QString pathA = safePath(args["path"].toString());
    const QString pathB = safePath(args.value("other_path").toString(args.value("destination").toString()));
    if (pathA.isEmpty() || pathB.isEmpty())
        return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager) {
        if (!m_sandboxManager->canAccess(pathA, FileSystemAccessMode::Read))
            return {callId, name(), true, "Sandbox policy denied read access."};
        if (!m_sandboxManager->canAccess(pathB, FileSystemAccessMode::Read))
            return {callId, name(), true, "Sandbox policy denied read access."};
    }

    QFile fileA(pathA);
    QFile fileB(pathB);
    if (!fileA.open(QIODevice::ReadOnly | QIODevice::Text))
        return {callId, name(), true, "Cannot open first file: " + fileA.errorString()};
    if (!fileB.open(QIODevice::ReadOnly | QIODevice::Text))
        return {callId, name(), true, "Cannot open second file: " + fileB.errorString()};

    const QStringList linesA = splitLinesKeepSimple(QString::fromUtf8(fileA.readAll()));
    const QStringList linesB = splitLinesKeepSimple(QString::fromUtf8(fileB.readAll()));
    const int maxLines = qMax(linesA.size(), linesB.size());
    const int maxChanges = qMax(1, args.value("max_results").toInt(200));

    QJsonArray changes;
    int added = 0;
    int removed = 0;
    int modified = 0;

    for (int i = 0; i < maxLines && changes.size() < maxChanges; ++i) {
        const bool hasA = i < linesA.size();
        const bool hasB = i < linesB.size();
        if (hasA && hasB) {
            if (linesA.at(i) != linesB.at(i)) {
                ++modified;
                QJsonObject item;
                item["line"] = i + 1;
                item["left"] = linesA.at(i);
                item["right"] = linesB.at(i);
                changes.append(item);
            }
        } else if (hasA) {
            ++removed;
            QJsonObject item;
            item["line"] = i + 1;
            item["left"] = linesA.at(i);
            item["right"] = QJsonValue();
            changes.append(item);
        } else {
            ++added;
            QJsonObject item;
            item["line"] = i + 1;
            item["left"] = QJsonValue();
            item["right"] = linesB.at(i);
            changes.append(item);
        }
    }

    QJsonObject result;
    result["path"] = args["path"].toString();
    result["other_path"] = args.value("other_path").toString(args.value("destination").toString());
    result["line_count_a"] = linesA.size();
    result["line_count_b"] = linesB.size();
    result["modified"] = modified;
    result["added"] = added;
    result["removed"] = removed;
    result["truncated"] = changes.size() >= maxChanges;
    result["changes"] = changes;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opTree(const QString &callId, const QJsonObject &args)
{
    const QString rootPath = safePath(args["path"].toString(args.value("base_dir").toString(".")));
    if (rootPath.isEmpty())
        return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager && !m_sandboxManager->canAccess(rootPath, FileSystemAccessMode::Read))
        return {callId, name(), true, "Sandbox policy denied read access."};

    QFileInfo rootInfo(rootPath);
    if (!rootInfo.exists())
        return {callId, name(), true, "Path does not exist."};

    const int maxDepth = qMax(0, args.value("max_depth").toInt(6));
    const int maxEntries = qMax(1, args.value("max_entries").toInt(400));
    const bool showHidden = args.value("show_hidden").toBool(false);

    QJsonArray entries;
    int visited = 0;

    std::function<void(const QString&, int)> walk = [&](const QString &path, int depth) {
        if (visited >= maxEntries || depth > maxDepth)
            return;

        QFileInfo info(path);
        QJsonObject item;
        item["path"] = m_root.relativeFilePath(path);
        item["name"] = info.fileName();
        item["depth"] = depth;
        item["is_directory"] = info.isDir();
        item["is_file"] = info.isFile();
        item["size"] = static_cast<qint64>(info.size());
        entries.append(item);
        ++visited;

        if (!info.isDir() || visited >= maxEntries || depth >= maxDepth)
            return;

        QDir dir(path);
        const auto entryInfos = dir.entryInfoList(
            QDir::AllEntries | QDir::NoDotAndDotDot | (showHidden ? QDir::Hidden : QDir::NoFilter),
            QDir::DirsFirst | QDir::Name);
        for (const QFileInfo &child : entryInfos) {
            if (visited >= maxEntries)
                break;
            walk(child.absoluteFilePath(), depth + 1);
        }
    };

    walk(rootInfo.absoluteFilePath(), 0);

    QJsonObject result;
    result["root"] = m_root.relativeFilePath(rootInfo.absoluteFilePath());
    result["max_depth"] = maxDepth;
    result["max_entries"] = maxEntries;
    result["returned"] = entries.size();
    result["truncated"] = visited >= maxEntries;
    result["entries"] = entries;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opCopyTree(const QString &callId, const QJsonObject &args)
{
    const QString src = safePath(args["path"].toString());
    const QString dest = safePath(args["destination"].toString());
    if (src.isEmpty() || dest.isEmpty())
        return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager) {
        if (!m_sandboxManager->canAccess(src, FileSystemAccessMode::Read))
            return {callId, name(), true, "Sandbox policy denied access: " + args["path"].toString()};
        if (!m_sandboxManager->canAccess(dest, FileSystemAccessMode::Write))
            return {callId, name(), true, "Sandbox policy denied access: " + args["destination"].toString()};
    }

    const QFileInfo srcInfo(src);
    if (!srcInfo.exists())
        return {callId, name(), true, "Source path does not exist."};

    const QString checkpointId = checkpointPaths(
        {args["path"].toString(), args["destination"].toString()},
        QStringLiteral("file_system copy_tree %1 -> %2")
            .arg(args["path"].toString(), args["destination"].toString()));

    if (!copyDirectoryRecursive(src, dest))
        return {callId, name(), true, "Failed to copy tree."};

    QJsonObject result;
    result["source"] = args["path"].toString();
    result["destination"] = args["destination"].toString();
    result["is_directory"] = srcInfo.isDir();
    result["checkpoint"] = checkpointId;
    result["copied"] = true;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opMoveTree(const QString &callId, const QJsonObject &args)
{
    const QString src = safePath(args["path"].toString());
    const QString dest = safePath(args["destination"].toString());
    if (src.isEmpty() || dest.isEmpty())
        return {callId, name(), true, "Path traversal denied."};
    if (m_sandboxManager) {
        if (!m_sandboxManager->canAccess(src, FileSystemAccessMode::Write))
            return {callId, name(), true, "Sandbox policy denied access: " + args["path"].toString()};
        if (!m_sandboxManager->canAccess(dest, FileSystemAccessMode::Write))
            return {callId, name(), true, "Sandbox policy denied access: " + args["destination"].toString()};
    }

    const QFileInfo srcInfo(src);
    if (!srcInfo.exists())
        return {callId, name(), true, "Source path does not exist."};

    const QString checkpointId = checkpointPaths(
        {args["path"].toString(), args["destination"].toString()},
        QStringLiteral("file_system move_tree %1 -> %2")
            .arg(args["path"].toString(), args["destination"].toString()));

    bool ok = false;
    if (srcInfo.isDir()) {
        ok = copyDirectoryRecursive(src, dest);
        if (ok) {
            QDir sourceDir(src);
            ok = sourceDir.removeRecursively();
        }
    } else {
        QDir().mkpath(QFileInfo(dest).absolutePath());
        if (QFile::exists(dest))
            QFile::remove(dest);
        ok = QFile::rename(src, dest);
    }

    if (!ok)
        return {callId, name(), true, "Failed to move tree."};

    QJsonObject result;
    result["source"] = args["path"].toString();
    result["destination"] = args["destination"].toString();
    result["is_directory"] = srcInfo.isDir();
    result["checkpoint"] = checkpointId;
    result["moved"] = true;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opWatchFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty())
        return {callId, name(), true, "Path traversal denied."};

    const bool recursive = args.value("recursive").toBool(false);
    if (!QFileInfo::exists(path))
        return {callId, name(), true, "Path does not exist."};

    FileService::instance()->watchFile(path, recursive);

    QJsonObject result;
    result["path"] = args["path"].toString();
    result["absolute_path"] = path;
    result["recursive"] = recursive;
    result["watched"] = true;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}

ToolResult FileSystemTool::opUnwatchFile(const QString &callId, const QJsonObject &args)
{
    const QString path = safePath(args["path"].toString());
    if (path.isEmpty())
        return {callId, name(), true, "Path traversal denied."};

    FileService::instance()->unwatchFile(path);

    QJsonObject result;
    result["path"] = args["path"].toString();
    result["absolute_path"] = path;
    result["watched"] = false;
    return {callId, name(), false, QJsonDocument(result).toJson(QJsonDocument::Compact)};
}
