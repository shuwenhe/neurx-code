#include "FolderTrustManager.h"
#include "FolderTrustDiscoveryService.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDirIterator>
#include <QRegularExpression>
#include <QSettings>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcFolderTrust, "neurx.security.foldertrust")

// Patterns that indicate suspicious content
static const QStringList SUSPICIOUS_PATTERNS = {
    "*.exe", "*.cmd", "*.bat", "*.ps1",      // Windows executables
    "*.sh", "*.bash", "*.zsh",               // Shell scripts
    "*.msi", "*.msp",                        // Windows installers
    "Makefile",                              // Build automation
    "CMakeLists.txt",                        // CMake build
    ".github/workflows/*.yml",               // GitHub Actions
    ".gitlab-ci.yml",                        // GitLab CI
    ".circleci/config.yml",                  // CircleCI
};

class FolderTrustManager::Impl {
public:
    struct TrustRecord {
        QString folderPath;
        bool isTrusted;
        QDateTime trustedAt;
        QString trustedBy;
        QString reason;
    };
    
    QHash<QString, TrustRecord> trustDatabase;
    QHash<QString, FolderTrustInfo> infoCache;
    QStringList monitoredFolders;
    QString configPath;
    
    QString getNormalizedPath(const QString& path) const {
        return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    }
    
    bool loadFromDisk() {
        QFile file(configPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        
        if (!doc.isObject()) {
            return false;
        }
        
        QJsonObject root = doc.object();
        QJsonArray trustedFoldersArray = root["trustedFolders"].toArray();
        
        for (const QJsonValue& val : trustedFoldersArray) {
            QJsonObject obj = val.toObject();
            TrustRecord rec;
            rec.folderPath = obj["path"].toString();
            rec.isTrusted = obj["trusted"].toBool();
            rec.trustedAt = QDateTime::fromString(obj["trustedAt"].toString(), Qt::ISODate);
            rec.trustedBy = obj["trustedBy"].toString("user");
            rec.reason = obj["reason"].toString();
            
            if (!rec.folderPath.isEmpty()) {
                trustDatabase[getNormalizedPath(rec.folderPath)] = rec;
            }
        }
        
        return true;
    }
    
    bool saveToDisk() const {
        QJsonObject root;
        QJsonArray trustedFoldersArray;
        
        for (const auto& rec : trustDatabase) {
            QJsonObject obj;
            obj["path"] = rec.folderPath;
            obj["trusted"] = rec.isTrusted;
            obj["trustedAt"] = rec.trustedAt.toString(Qt::ISODate);
            obj["trustedBy"] = rec.trustedBy;
            obj["reason"] = rec.reason;
            trustedFoldersArray.append(obj);
        }
        
        root["trustedFolders"] = trustedFoldersArray;
        
        QJsonDocument doc(root);
        QFile file(configPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            return false;
        }
        
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }
};

FolderTrustManager* FolderTrustManager::instance()
{
    static FolderTrustManager s_instance;
    return &s_instance;
}

FolderTrustManager::FolderTrustManager()
    : m_impl(std::make_unique<Impl>())
{
    // Initialize trust storage path
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(configDir);
    m_impl->configPath = configDir + "/folder_trust.json";
    
    // Load existing trust decisions
    loadTrustDecisions();
    
    qCInfo(lcFolderTrust) << "FolderTrustManager initialized";
}

FolderTrustManager::~FolderTrustManager() = default;

bool FolderTrustManager::isFolderTrusted(const QString& folderPath)
{
    QMutexLocker locker(&m_mutex);
    
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    
    // Check cache first
    if (m_impl->trustDatabase.contains(normalizedPath)) {
        return m_impl->trustDatabase[normalizedPath].isTrusted;
    }
    
    // Check if it's a system folder (implicitly trusted)
    QFileInfo fi(normalizedPath);
    if (isSystemFolder(normalizedPath)) {
        return true;
    }
    
    // Not explicitly trusted and not system - return false
    return false;
}

FolderTrustInfo FolderTrustManager::getTrustInfo(const QString& folderPath)
{
    QMutexLocker locker(&m_mutex);
    
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    
    if (m_impl->infoCache.contains(normalizedPath)) {
        return m_impl->infoCache[normalizedPath];
    }
    
    FolderTrustInfo info;
    info.folderPath = normalizedPath;
    info.isTrusted = m_impl->trustDatabase.contains(normalizedPath)
        ? m_impl->trustDatabase[normalizedPath].isTrusted
        : isSystemFolder(normalizedPath);
    info.isSystemFolder = isSystemFolder(normalizedPath);
    info.containsSuspiciousContent = !scanForSuspiciousPatterns(normalizedPath).isEmpty();
    
    if (m_impl->trustDatabase.contains(normalizedPath)) {
        const auto& rec = m_impl->trustDatabase[normalizedPath];
        info.trustedAt = rec.trustedAt;
        info.trustedBy = rec.trustedBy;
        info.reason = rec.reason;
    }
    
    m_impl->infoCache[normalizedPath] = info;
    return info;
}

void FolderTrustManager::markFolderAsTrusted(const QString& folderPath, const QString& reason)
{
    QMutexLocker locker(&m_mutex);
    
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    
    Impl::TrustRecord rec;
    rec.folderPath = normalizedPath;
    rec.isTrusted = true;
    rec.trustedAt = QDateTime::currentDateTime();
    rec.trustedBy = "user";
    rec.reason = reason.isEmpty() ? "Manually marked as trusted" : reason;
    
    m_impl->trustDatabase[normalizedPath] = rec;
    m_impl->infoCache.remove(normalizedPath);  // Invalidate cache
    
    saveTrustDecisions();
    
    qCInfo(lcFolderTrust) << "Folder marked as trusted:" << normalizedPath << "(" << reason << ")";
    emit folderTrustChanged(normalizedPath, true);
}

void FolderTrustManager::markFolderAsUntrusted(const QString& folderPath)
{
    QMutexLocker locker(&m_mutex);
    
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    
    Impl::TrustRecord rec;
    rec.folderPath = normalizedPath;
    rec.isTrusted = false;
    rec.trustedAt = QDateTime::currentDateTime();
    rec.trustedBy = "user";
    rec.reason = "Manually marked as untrusted";
    
    m_impl->trustDatabase[normalizedPath] = rec;
    m_impl->infoCache.remove(normalizedPath);
    
    saveTrustDecisions();
    
    qCInfo(lcFolderTrust) << "Folder marked as untrusted:" << normalizedPath;
    emit folderTrustChanged(normalizedPath, false);
}

void FolderTrustManager::revokeTrustDecision(const QString& folderPath)
{
    QMutexLocker locker(&m_mutex);
    
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    m_impl->trustDatabase.remove(normalizedPath);
    m_impl->infoCache.remove(normalizedPath);
    
    saveTrustDecisions();
    
    qCInfo(lcFolderTrust) << "Trust decision revoked for:" << normalizedPath;
}

bool FolderTrustManager::performTrustDiscovery(const QString& folderPath)
{
    QMutexLocker locker(&m_mutex);
    
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    
    qCInfo(lcFolderTrust) << "Starting trust discovery for:" << normalizedPath;

    const FolderDiscoveryResults discovery = FolderTrustDiscoveryService::discover(normalizedPath);
    QStringList suspiciousItems = scanForSuspiciousPatterns(normalizedPath);
    suspiciousItems.append(discovery.securityWarnings);
    suspiciousItems.removeDuplicates();

    if (!discovery.commands.isEmpty() || !discovery.skills.isEmpty() || !discovery.agents.isEmpty()
        || !discovery.mcps.isEmpty() || !discovery.settings.isEmpty()) {
        qCInfo(lcFolderTrust) << "Discovery summary for" << normalizedPath
                              << "commands=" << discovery.commands.size()
                              << "skills=" << discovery.skills.size()
                              << "agents=" << discovery.agents.size()
                              << "mcps=" << discovery.mcps.size()
                              << "settings=" << discovery.settings.size();
    }

    if (!suspiciousItems.isEmpty()) {
        FolderTrustInfo info;
        info.folderPath = normalizedPath;
        info.containsSuspiciousContent = true;
        info.reason = QStringLiteral("Discovery found suspicious items");
        m_impl->infoCache[normalizedPath] = info;
        
        qCWarning(lcFolderTrust) << "Suspicious content found in:" << normalizedPath;
        emit suspiciousContentFound(normalizedPath, suspiciousItems);
        emit trustDecisionRequired(normalizedPath, suspiciousItems);
        return false;
    }

    FolderTrustInfo info;
    info.folderPath = normalizedPath;
    info.isTrusted = true;
    info.isSystemFolder = isSystemFolder(normalizedPath);
    info.reason = QStringLiteral("Passed trust discovery");
    m_impl->infoCache[normalizedPath] = info;

    emit folderScanned(normalizedPath);
    return true;
}

QStringList FolderTrustManager::scanForSuspiciousPatterns(const QString& folderPath)
{
    QStringList results;
    QDir dir(folderPath);
    
    if (!dir.exists()) {
        return results;
    }
    
    // Limit scan depth to avoid performance issues
    QDirIterator it(folderPath, QDir::Files, QDirIterator::NoIteratorFlags);
    int fileCount = 0;
    
    while (it.hasNext() && fileCount < 500) {
        it.next();
        QString fileName = it.fileName();
        QString filePath = it.filePath();
        
        // Check against suspicious patterns
        for (const QString& pattern : SUSPICIOUS_PATTERNS) {
            QRegularExpression regex(QRegularExpression::wildcardToRegularExpression(pattern),
                                    QRegularExpression::CaseInsensitiveOption);
            if (regex.match(fileName).hasMatch()) {
                results.append(filePath);
                break;
            }
        }
        
        fileCount++;
    }
    
    return results;
}

QStringList FolderTrustManager::scanForExecutables(const QString& folderPath)
{
    QStringList results;
    QDir dir(folderPath);
    
    if (!dir.exists()) {
        return results;
    }
    
    QDirIterator it(folderPath, {"*.exe", "*.sh", "*.bat", "*.ps1", "*.cmd"},
                   QDir::Files, QDirIterator::NoIteratorFlags);
    
    while (it.hasNext()) {
        results.append(it.next());
    }
    
    return results;
}

bool FolderTrustManager::shouldPromptForTrust(const QString& folderPath)
{
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    
    // Already trusted or explicitly untrusted - no prompt
    if (m_impl->trustDatabase.contains(normalizedPath)) {
        return false;
    }
    
    // System folders - no prompt
    if (isSystemFolder(normalizedPath)) {
        return false;
    }
    
    // Folder with suspicious content - prompt
    return !FolderTrustDiscoveryService::discover(normalizedPath).securityWarnings.isEmpty()
        || !scanForSuspiciousPatterns(normalizedPath).isEmpty();
}

void FolderTrustManager::recordTrustDecision(const QString& folderPath, bool trusted, const QString& reason)
{
    if (trusted) {
        markFolderAsTrusted(folderPath, reason);
    } else {
        markFolderAsUntrusted(folderPath);
    }
}

bool FolderTrustManager::saveTrustDecisions()
{
    return m_impl->saveToDisk();
}

bool FolderTrustManager::loadTrustDecisions()
{
    return m_impl->loadFromDisk();
}

QString FolderTrustManager::getTrustStoragePath() const
{
    return m_impl->configPath;
}

void FolderTrustManager::clearTrustData()
{
    QMutexLocker locker(&m_mutex);
    m_impl->trustDatabase.clear();
    m_impl->infoCache.clear();
    saveTrustDecisions();
}

QStringList FolderTrustManager::getTrustedFolders() const
{
    QMutexLocker locker(&m_mutex);
    
    QStringList results;
    for (const auto& rec : m_impl->trustDatabase) {
        if (rec.isTrusted) {
            results.append(rec.folderPath);
        }
    }
    return results;
}

QStringList FolderTrustManager::getUntrustedFolders() const
{
    QMutexLocker locker(&m_mutex);
    
    QStringList results;
    for (const auto& rec : m_impl->trustDatabase) {
        if (!rec.isTrusted) {
            results.append(rec.folderPath);
        }
    }
    return results;
}

void FolderTrustManager::markMultipleFoldersAsTrusted(const QStringList& folders)
{
    for (const QString& folder : folders) {
        markFolderAsTrusted(folder, "Batch trust operation");
    }
}

bool FolderTrustManager::isMonitoring(const QString& folderPath) const
{
    return m_impl->monitoredFolders.contains(m_impl->getNormalizedPath(folderPath));
}

void FolderTrustManager::startMonitoring(const QString& folderPath)
{
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    if (!m_impl->monitoredFolders.contains(normalizedPath)) {
        m_impl->monitoredFolders.append(normalizedPath);
        qCInfo(lcFolderTrust) << "Started monitoring folder:" << normalizedPath;
    }
}

void FolderTrustManager::stopMonitoring(const QString& folderPath)
{
    QString normalizedPath = m_impl->getNormalizedPath(folderPath);
    m_impl->monitoredFolders.removeAll(normalizedPath);
    qCInfo(lcFolderTrust) << "Stopped monitoring folder:" << normalizedPath;
}

// Helper function to check if a folder is a system folder
bool FolderTrustManager::isSystemFolder(const QString& folderPath) const
{
    // Implicitly trust system paths
    if (folderPath.startsWith("/usr") || folderPath.startsWith("/opt") || 
        folderPath.startsWith("/System") || folderPath.startsWith("/Applications") ||
        folderPath.startsWith("C:\\Windows") || folderPath.startsWith("C:\\Program Files")) {
        return true;
    }
    return false;
}
