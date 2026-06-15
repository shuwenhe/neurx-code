#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QSignalSpy>

#include "tools/FileSystemTool.h"
#include "services/FileService.h"

class TestFileSystemTool : public QObject {
    Q_OBJECT

private:
    static QString readTextFile(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return {};
        return QString::fromUtf8(file.readAll());
    }

    static void writeTextFile(const QString &path, const QString &content)
    {
        QFile file(path);
        QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Text), qPrintable(file.errorString()));
        QTextStream out(&file);
        out << content;
        QVERIFY2(file.flush(), "Failed to flush file");
        file.close();
    }

    static QString makeWorkspace()
    {
        static QTemporaryDir baseDir;
        if (!baseDir.isValid())
            return {};

        static int counter = 0;
        const QString workspace = QDir(baseDir.path()).filePath(
            QStringLiteral("%1-%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(++counter));
        QDir().mkpath(workspace);

        QProcess git;
        git.setWorkingDirectory(workspace);
        git.start(QStringLiteral("git"), {QStringLiteral("init"), QStringLiteral("-q")});
        if (!git.waitForFinished(30000) || git.exitCode() != 0)
            return {};

        return workspace;
    }

private slots:
    void testReadMany()
    {
        const QString workspace = makeWorkspace();
        QVERIFY(!workspace.isEmpty());
        writeTextFile(QDir(workspace).filePath("a.txt"), "alpha\nbeta\n");
        writeTextFile(QDir(workspace).filePath("b.txt"), "one\ntwo\nthree\n");

        FileSystemTool tool(workspace);
        QJsonObject args;
        args["operation"] = "read_many";
        QJsonArray paths;
        paths.append(QStringLiteral("a.txt"));
        paths.append(QStringLiteral("b.txt"));
        args["paths"] = paths;
        args["start_line"] = 2;
        args["end_line"] = 2;

        const ToolResult result = tool.execute("call-read-many", args);
        QVERIFY(!result.isError);
        const QJsonDocument doc = QJsonDocument::fromJson(result.content.toUtf8());
        QVERIFY(doc.isObject());
        const QJsonObject payload = doc.object();
        QCOMPARE(payload.value("count").toInt(), 2);
        const QJsonArray files = payload.value("files").toArray();
        QCOMPARE(files.size(), 2);
        QCOMPARE(files.at(0).toObject().value("content").toString(), QStringLiteral("beta"));
        QCOMPARE(files.at(1).toObject().value("content").toString(), QStringLiteral("two"));
    }

    void testReplaceBatchDryRunAndApply()
    {
        const QString workspace = makeWorkspace();
        QVERIFY(!workspace.isEmpty());
        const QString path = QDir(workspace).filePath("notes.txt");
        writeTextFile(path, "hello world\nhello agent\n");

        FileSystemTool tool(workspace);

        QJsonObject dryRunArgs;
        dryRunArgs["operation"] = "replace_batch";
        dryRunArgs["dry_run"] = true;
        QJsonArray files;
        QJsonObject replaceSpec;
        replaceSpec["path"] = "notes.txt";
        replaceSpec["search"] = "hello";
        replaceSpec["replacement"] = "hi";
        files.append(replaceSpec);
        dryRunArgs["files"] = files;

        const ToolResult dryRun = tool.execute("call-replace-batch-dry", dryRunArgs);
        QVERIFY(!dryRun.isError);
        const QJsonDocument dryDoc = QJsonDocument::fromJson(dryRun.content.toUtf8());
        QVERIFY(dryDoc.isObject());
        const QJsonObject dryPayload = dryDoc.object();
        QCOMPARE(dryPayload.value("changed_files").toInt(), 1);
        QCOMPARE(dryPayload.value("preview").toArray().size(), 1);
        QCOMPARE(readTextFile(path), QStringLiteral("hello world\nhello agent\n"));

        QJsonObject applyArgs = dryRunArgs;
        applyArgs["dry_run"] = false;
        const ToolResult apply = tool.execute("call-replace-batch-apply", applyArgs);
        QVERIFY(!apply.isError);
        const QJsonDocument applyDoc = QJsonDocument::fromJson(apply.content.toUtf8());
        QVERIFY(applyDoc.isObject());
        const QJsonObject applyPayload = applyDoc.object();
        QCOMPARE(applyPayload.value("changed_files").toInt(), 1);
        QCOMPARE(readTextFile(path), QStringLiteral("hi world\nhi agent\n"));
    }

    void testCopyTreeAndMoveTree()
    {
        const QString workspace = makeWorkspace();
        QVERIFY(!workspace.isEmpty());
        QDir root(workspace);
        root.mkpath("src/nested");
        writeTextFile(root.filePath("src/root.txt"), "root\n");
        writeTextFile(root.filePath("src/nested/item.txt"), "item\n");

        FileSystemTool tool(workspace);

        QJsonObject copyArgs;
        copyArgs["operation"] = "copy_tree";
        copyArgs["path"] = "src";
        copyArgs["destination"] = "copy";
        const ToolResult copy = tool.execute("call-copy-tree", copyArgs);
        QVERIFY(!copy.isError);
        QVERIFY(QFileInfo::exists(root.filePath("copy/root.txt")));
        QVERIFY(QFileInfo::exists(root.filePath("copy/nested/item.txt")));

        QJsonObject moveArgs;
        moveArgs["operation"] = "move_tree";
        moveArgs["path"] = "copy";
        moveArgs["destination"] = "moved";
        const ToolResult move = tool.execute("call-move-tree", moveArgs);
        QVERIFY(!move.isError);
        QVERIFY(!QFileInfo::exists(root.filePath("copy/root.txt")));
        QVERIFY(QFileInfo::exists(root.filePath("moved/root.txt")));
        QVERIFY(QFileInfo::exists(root.filePath("moved/nested/item.txt")));
    }

    void testPreviewAndApplyPatch()
    {
        const QString workspace = makeWorkspace();
        QVERIFY(!workspace.isEmpty());
        const QString path = QDir(workspace).filePath("patchme.txt");
        writeTextFile(path, "alpha\n");

        FileSystemTool tool(workspace);

        const QString patchText = QStringLiteral(
            "diff --git a/patchme.txt b/patchme.txt\n"
            "--- a/patchme.txt\n"
            "+++ b/patchme.txt\n"
            "@@ -1 +1 @@\n"
            "-alpha\n"
            "+beta\n");

        QJsonObject previewArgs;
        previewArgs["operation"] = "preview_patch";
        previewArgs["patch"] = patchText;
        const ToolResult preview = tool.execute("call-preview-patch", previewArgs);
        if (preview.isError)
            qWarning().noquote() << "preview_patch error:" << preview.content;
        QVERIFY(!preview.isError);
        QVERIFY(preview.content.contains("Patch is applicable"));

        QJsonObject applyArgs = previewArgs;
        applyArgs["operation"] = "apply_patch";
        const ToolResult apply = tool.execute("call-apply-patch", applyArgs);
        QVERIFY(!apply.isError);
        QCOMPARE(readTextFile(path), QStringLiteral("beta\n"));
    }

    void testDiffTreeWatchUnwatch()
    {
        const QString workspace = makeWorkspace();
        QVERIFY(!workspace.isEmpty());

        QDir root(workspace);
        root.mkpath("dir/sub");
        writeTextFile(root.filePath("left.txt"), "same\nleft\n");
        writeTextFile(root.filePath("right.txt"), "same\nright\n");
        writeTextFile(root.filePath("dir/sub/leaf.txt"), "leaf\n");

        FileSystemTool tool(workspace);

        QJsonObject diffArgs;
        diffArgs["operation"] = "diff";
        diffArgs["path"] = "left.txt";
        diffArgs["other_path"] = "right.txt";
        const ToolResult diff = tool.execute("call-diff", diffArgs);
        QVERIFY(!diff.isError);
        const QJsonDocument diffDoc = QJsonDocument::fromJson(diff.content.toUtf8());
        QVERIFY(diffDoc.isObject());
        const QJsonObject diffPayload = diffDoc.object();
        QVERIFY(diffPayload.value("modified").toInt() >= 1);
        const QJsonArray changes = diffPayload.value("changes").toArray();
        QVERIFY(!changes.isEmpty());
        QCOMPARE(changes.first().toObject().value("left").toString(), QStringLiteral("left"));
        QCOMPARE(changes.first().toObject().value("right").toString(), QStringLiteral("right"));

        QJsonObject treeArgs;
        treeArgs["operation"] = "tree";
        treeArgs["path"] = ".";
        treeArgs["max_depth"] = 4;
        treeArgs["max_entries"] = 20;
        const ToolResult tree = tool.execute("call-tree", treeArgs);
        QVERIFY(!tree.isError);
        const QJsonDocument treeDoc = QJsonDocument::fromJson(tree.content.toUtf8());
        QVERIFY(treeDoc.isObject());
        const QJsonObject treePayload = treeDoc.object();
        const QJsonArray entries = treePayload.value("entries").toArray();
        QVERIFY(entries.size() >= 4);

        FileService *service = FileService::instance();
        QVERIFY(service != nullptr);
        QSignalSpy watchedSpy(service, &FileService::fileWatched);
        QSignalSpy unwatchedSpy(service, &FileService::fileUnwatched);

        QJsonObject watchArgs;
        watchArgs["operation"] = "watch";
        watchArgs["path"] = "dir";
        watchArgs["recursive"] = true;
        const ToolResult watch = tool.execute("call-watch", watchArgs);
        QVERIFY(!watch.isError);
        QTRY_VERIFY_WITH_TIMEOUT(service->isWatching(QDir(workspace).filePath("dir")), 1000);
        QVERIFY(watchedSpy.count() >= 1);

        QJsonObject unwatchArgs;
        unwatchArgs["operation"] = "unwatch";
        unwatchArgs["path"] = "dir";
        const ToolResult unwatch = tool.execute("call-unwatch", unwatchArgs);
        QVERIFY(!unwatch.isError);
        QTRY_VERIFY_WITH_TIMEOUT(!service->isWatching(QDir(workspace).filePath("dir")), 1000);
        QVERIFY(unwatchedSpy.count() >= 1);
    }
};

QTEST_MAIN(TestFileSystemTool)
#include "tst_FileSystemTool.moc"
