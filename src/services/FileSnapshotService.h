#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QStringList>
#include <QDateTime>

/**
 * @class FileSnapshotService
 * @brief Manages snapshots of file contents for rollback support
 */
class FileSnapshotService : public QObject {
    Q_OBJECT

public:
    static FileSnapshotService* instance();

    struct Snapshot {
        QString id;
        QDateTime timestamp;
        QMap<QString, QString> fileContents; // filePath -> content
    };

    /**
     * @brief Take a snapshot of specific files
     */
    QString takeSnapshot(const QStringList& filePaths);

    /**
     * @brief Restore files from a snapshot
     */
    bool restoreSnapshot(const QString& snapshotId);

    /**
     * @brief Discard a snapshot
     */
    void discardSnapshot(const QString& snapshotId);

    /**
     * @brief List all snapshots
     */
    QStringList listSnapshots() const;

private:
    FileSnapshotService();
    ~FileSnapshotService() override;

    QMap<QString, Snapshot> m_snapshots;
};

