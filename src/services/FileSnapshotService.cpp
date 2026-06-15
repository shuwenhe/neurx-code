#include "services/FileSnapshotService.h"
#include "services/FileService.h"
#include <QUuid>
#include <QDebug>

FileSnapshotService* FileSnapshotService::instance() {
    static FileSnapshotService* inst = new FileSnapshotService();
    return inst;
}

FileSnapshotService::FileSnapshotService() {}
FileSnapshotService::~FileSnapshotService() = default;

QString FileSnapshotService::takeSnapshot(const QStringList& filePaths) {
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    Snapshot snapshot;
    snapshot.id = id;
    snapshot.timestamp = QDateTime::currentDateTime();

    FileService* fs = FileService::instance();
    for (const QString& path : filePaths) {
        QString content = fs->readFile(path);
        // Even if empty or read failed, we track it as "intended to be original"
        snapshot.fileContents[path] = content;
    }

    m_snapshots[id] = snapshot;
    qInfo() << "[FileSnapshotService] Created snapshot:" << id << "files:" << filePaths.size();
    return id;
}

bool FileSnapshotService::restoreSnapshot(const QString& snapshotId) {
    if (!m_snapshots.contains(snapshotId)) {
        qWarning() << "[FileSnapshotService] Snapshot not found:" << snapshotId;
        return false;
    }

    const Snapshot& snapshot = m_snapshots[snapshotId];
    FileService* fs = FileService::instance();
    bool allSuccess = true;

    for (auto it = snapshot.fileContents.begin(); it != snapshot.fileContents.end(); ++it) {
        const QString& path = it.key();
        const QString& content = it.value();

        if (!fs->writeFileAtomic(path, content.toUtf8())) {
            qCritical() << "[FileSnapshotService] Failed to restore file:" << path;
            allSuccess = false;
        }
    }

    qInfo() << "[FileSnapshotService] Restored snapshot:" << snapshotId << "success:" << allSuccess;
    return allSuccess;
}

void FileSnapshotService::discardSnapshot(const QString& snapshotId) {
    m_snapshots.remove(snapshotId);
}

QStringList FileSnapshotService::listSnapshots() const {
    return m_snapshots.keys();
}
