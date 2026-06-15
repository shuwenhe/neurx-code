#include "FileService.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringConverter>
#include <QSaveFile>
#include <QSet>

namespace {

QString normalizedEncoding(const QString& encoding) {
    if (encoding.isEmpty()) {
        return QStringLiteral("UTF-8");
    }
    return encoding.trimmed().toUpper();
}

QString decodeText(const QByteArray& content, const QString& encoding) {
    const QString enc = normalizedEncoding(encoding);

    if (enc == QStringLiteral("UTF-16LE")) {
        QStringDecoder decoder(QStringConverter::Utf16LE);
        return decoder.decode(content);
    }
    if (enc == QStringLiteral("UTF-16BE")) {
        QStringDecoder decoder(QStringConverter::Utf16BE);
        return decoder.decode(content);
    }
    if (enc == QStringLiteral("LATIN1") || enc == QStringLiteral("ISO-8859-1")) {
        QStringDecoder decoder(QStringConverter::Latin1);
        return decoder.decode(content);
    }

    QStringDecoder decoder(QStringConverter::Utf8);
    return decoder.decode(content);
}

QByteArray encodeText(const QString& content, const QString& encoding) {
    const QString enc = normalizedEncoding(encoding);

    if (enc == QStringLiteral("UTF-16LE")) {
        QStringEncoder encoder(QStringConverter::Utf16LE);
        return encoder.encode(content);
    }
    if (enc == QStringLiteral("UTF-16BE")) {
        QStringEncoder encoder(QStringConverter::Utf16BE);
        return encoder.encode(content);
    }
    if (enc == QStringLiteral("LATIN1") || enc == QStringLiteral("ISO-8859-1")) {
        QStringEncoder encoder(QStringConverter::Latin1);
        return encoder.encode(content);
    }

    return content.toUtf8();
}

}  // namespace

class FileService::Impl {
public:
    QFileSystemWatcher watcher;
    QStringList recentFiles;
    QSet<QString> recursiveRoots;
    QSet<QString> watchedPaths;
    static constexpr int MAX_RECENT = 50;

    static QString normalizePath(const QString& path) {
        return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    }

    bool addPath(const QString& path) {
        const QString normalized = normalizePath(path);
        if (normalized.isEmpty())
            return false;
        if (watchedPaths.contains(normalized))
            return true;
        const bool added = watcher.addPath(normalized);
        if (added)
            watchedPaths.insert(normalized);
        return added;
    }

    void removePath(const QString& path) {
        const QString normalized = normalizePath(path);
        if (normalized.isEmpty())
            return;
        watcher.removePath(normalized);
        watchedPaths.remove(normalized);
    }

    void addRecursiveRoot(const QString& root) {
        const QString normalizedRoot = normalizePath(root);
        if (normalizedRoot.isEmpty() || recursiveRoots.contains(normalizedRoot))
            return;

        recursiveRoots.insert(normalizedRoot);
        addPath(normalizedRoot);

        QDirIterator it(normalizedRoot, QDir::AllEntries | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            addPath(it.next());
        }
    }

    void removeRecursiveRoot(const QString& root) {
        const QString normalizedRoot = normalizePath(root);
        if (normalizedRoot.isEmpty())
            return;

        recursiveRoots.remove(normalizedRoot);
        const auto removeIfUnderRoot = [&](const QStringList& list) {
            for (const QString& watched : list) {
                if (watched == normalizedRoot
                    || watched.startsWith(normalizedRoot + QDir::separator())) {
                    watcher.removePath(watched);
                    watchedPaths.remove(watched);
                }
            }
        };
        removeIfUnderRoot(watcher.files());
        removeIfUnderRoot(watcher.directories());
    }

    bool isWatched(const QString& path) const {
        const QString normalized = normalizePath(path);
        return watchedPaths.contains(normalized) || recursiveRoots.contains(normalized);
    }
    
    QString detectEncodingInternal(const QByteArray& data) {
        // Simple encoding detection - BOM check
        if (data.startsWith("\xEF\xBB\xBF")) {
            return "UTF-8 BOM";
        } else if (data.startsWith("\xFF\xFE")) {
            return "UTF-16LE";
        } else if (data.startsWith("\xFE\xFF")) {
            return "UTF-16BE";
        }
        
        // Default to UTF-8
        return "UTF-8";
    }
};

FileService* FileService::instance() {
    static FileService s_instance;
    return &s_instance;
}

FileService::FileService()
    : m_impl(std::make_unique<Impl>()) {
    connect(&m_impl->watcher, &QFileSystemWatcher::fileChanged,
            this, [this](const QString& path) {
                emit fileChanged(path);
                if (QFileInfo::exists(path)) {
                    m_impl->addPath(path);
                }
            });
    connect(&m_impl->watcher, &QFileSystemWatcher::directoryChanged,
            this, [this](const QString& path) {
                emit directoryChanged(path);
                for (const QString& root : std::as_const(m_impl->recursiveRoots)) {
                    if (path == root
                        || path.startsWith(root + QDir::separator())
                        || root.startsWith(path + QDir::separator())) {
                        m_impl->addRecursiveRoot(root);
                    }
                }
            });
}

FileService::~FileService() = default;

bool FileService::exists(const QString& path) const {
    return QFileInfo::exists(path);
}

FileInfo FileService::getFileInfo(const QString& path) const {
    QFileInfo fileInfo(path);
    
    FileInfo info;
    info.path = fileInfo.absoluteFilePath();
    info.name = fileInfo.fileName();
    info.extension = fileInfo.suffix();
    info.size = fileInfo.size();
    info.modified = fileInfo.lastModified().toMSecsSinceEpoch();
    info.isDirectory = fileInfo.isDir();
    info.isSymlink = fileInfo.isSymLink();
    info.encoding = "UTF-8";
    
    return info;
}

QByteArray FileService::readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    
    QByteArray content = file.readAll();
    file.close();
    
    return content;
}

bool FileService::writeFile(const QString& path, const QByteArray& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    
    file.write(content);
    file.close();
    
    return true;
}

bool FileService::deleteFile(const QString& path) {
    return QFile::remove(path);
}

bool FileService::moveFile(const QString& source, const QString& destination) {
    return QFile::rename(source, destination);
}

bool FileService::copyFile(const QString& source, const QString& destination) {
    return copyFile(source, destination, /*recursive*/ false);
}

bool FileService::copyFile(const QString& source, const QString& destination, bool recursive) {
    QFileInfo srcInfo(source);
    if (srcInfo.isDir()) {
        if (!recursive) return false;

        QDir targetDir(destination);
        if (!targetDir.exists()) {
            if (!QDir().mkpath(destination)) return false;
        }

        QDir srcDir(source);
        QFileInfoList entries = srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo& entry : entries) {
            QString relPath = srcDir.relativeFilePath(entry.filePath());
            QString destPath = QDir(destination).filePath(relPath);
            if (entry.isDir()) {
                if (!copyFile(entry.filePath(), destPath, true)) return false;
            } else {
                QDir().mkpath(QFileInfo(destPath).absolutePath());
                if (!QFile::copy(entry.filePath(), destPath)) return false;
            }
        }

        return true;
    }

    // Regular file
    QDir().mkpath(QFileInfo(destination).absolutePath());
    return QFile::copy(source, destination);
}

FileInfo FileService::getMetadata(const QString& path) const {
    return getFileInfo(path);
}

QString FileService::canonicalize(const QString& path) const {
    QFileInfo fi(path);
    QString canonical = fi.canonicalFilePath();
    if (canonical.isEmpty()) return fi.absoluteFilePath();
    return canonical;
}

QString FileService::joinPaths(const QString& base, const QString& relative) const {
    QDir baseDir(base);
    return baseDir.filePath(relative);
}

QString FileService::parentPath(const QString& path) const {
    QFileInfo fi(path);
    return fi.dir().absolutePath();
}

bool FileService::writeFileAtomic(const QString& path, const QByteArray& content) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    qint64 written = file.write(content);
    if (written != content.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

bool FileService::removePath(const QString& path, bool recursive, bool force) {
    QFileInfo fi(path);
    if (!fi.exists()) return true;

    if (fi.isDir()) {
        if (!recursive) {
            QDir dir(path);
            return dir.rmdir(path);
        }
        // recursive delete
        QDir dir(path);
        // If force, attempt to change permissions on files
        if (force) {
            QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries, QDir::DirsFirst);
            for (const QFileInfo& entry : entries) {
                if (entry.isDir()) continue;
                QFile::setPermissions(entry.filePath(), QFile::ReadOwner | QFile::WriteOwner);
            }
        }
        return dir.removeRecursively();
    }

    // file
    if (force) {
        QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
    }
    return QFile::remove(path);
}

bool FileService::createDirectory(const QString& path) {
    return QDir().mkpath(path);
}

QList<FileInfo> FileService::listDirectory(const QString& path, bool recursive) {
    QList<FileInfo> results;
    
    QDir dir(path);
    if (!dir.exists()) {
        return results;
    }
    
    QStringList filters = QStringList() << "*";
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    
    QFileInfoList entries = dir.entryInfoList();
    for (const auto& entry : entries) {
        FileInfo info;
        info.path = entry.absoluteFilePath();
        info.name = entry.fileName();
        info.extension = entry.suffix();
        info.size = entry.size();
        info.modified = entry.lastModified().toMSecsSinceEpoch();
        info.isDirectory = entry.isDir();
        info.isSymlink = entry.isSymLink();
        
        results.append(info);
        
        if (recursive && entry.isDir() && !entry.isSymLink()) {
            auto subResults = listDirectory(entry.absoluteFilePath(), true);
            results.append(subResults);
        }
    }
    
    return results;
}

QStringList FileService::findFiles(const QString& directory, const QString& pattern) {
    QStringList results;
    
    QDir dir(directory);
    if (!dir.exists()) {
        return results;
    }
    
    QDirIterator it(directory,
                    QStringList(pattern),
                    QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        results.append(it.next());
    }
    
    return results;
}

QString FileService::detectEncoding(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return "UTF-8";
    }
    
    QByteArray data = file.read(4096);  // Read first 4KB
    file.close();
    
    return m_impl->detectEncodingInternal(data);
}

QString FileService::readFileAsText(const QString& path, const QString& encoding) {
    QByteArray content = readFile(path);
    if (content.isEmpty()) {
        return QString();
    }

    return decodeText(content, encoding);
}

bool FileService::writeFileAsText(const QString& path, const QString& content,
                                 const QString& encoding) {
    return writeFile(path, encodeText(content, encoding));
}

void FileService::watchFile(const QString& path, bool recursive) {
    const QString normalized = m_impl->normalizePath(path);
    if (normalized.isEmpty())
        return;

    QFileInfo info(normalized);
    if (recursive && info.isDir()) {
        m_impl->addRecursiveRoot(normalized);
        emit fileWatched(normalized);
        return;
    }

    if (m_impl->addPath(normalized)) {
        emit fileWatched(normalized);
    }
}

void FileService::unwatchFile(const QString& path) {
    const QString normalized = m_impl->normalizePath(path);
    if (normalized.isEmpty())
        return;

    if (m_impl->recursiveRoots.contains(normalized) || QFileInfo(normalized).isDir()) {
        m_impl->removeRecursiveRoot(normalized);
        emit fileUnwatched(normalized);
        return;
    }

    if (m_impl->isWatched(normalized)) {
        m_impl->removePath(normalized);
        emit fileUnwatched(normalized);
    }
}

bool FileService::isWatching(const QString& path) const {
    const QString normalized = m_impl->normalizePath(path);
    return m_impl->isWatched(normalized) || m_impl->recursiveRoots.contains(normalized);
}

QStringList FileService::getRecentFiles(int maxCount) {
    return m_impl->recentFiles.mid(
        qMax(0, m_impl->recentFiles.size() - maxCount)
    );
}

void FileService::addRecentFile(const QString& path) {
    // Remove duplicate if exists
    m_impl->recentFiles.removeAll(path);
    
    // Add to end
    m_impl->recentFiles.append(path);
    
    // Trim if too large
    if (m_impl->recentFiles.size() > m_impl->MAX_RECENT) {
        m_impl->recentFiles.removeFirst();
    }
}

void FileService::clearRecentFiles() {
    m_impl->recentFiles.clear();
}

qint64 FileService::getFileSize(const QString& path) const {
    return QFileInfo(path).size();
}

QString FileService::getFileExtension(const QString& path) const {
    return QFileInfo(path).suffix();
}

QString FileService::getFileName(const QString& path) const {
    return QFileInfo(path).fileName();
}
