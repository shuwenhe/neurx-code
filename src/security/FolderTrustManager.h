#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QRecursiveMutex>
#include <memory>

/**
 * @class FolderTrustManager
 * @brief Manages folder trust discovery and verification
 * 
 * Features:
 * - Folder trust state management
 * - Trust decision persistence
 * - Automatic trust scanning
 * - User trust prompts
 * - Trusted workspace tracking
 */

struct FolderTrustInfo {
    QString folderPath;
    bool isTrusted = false;
    QDateTime trustedAt;
    QString trustedBy;
    QString reason;
    bool isSystemFolder = false;
    bool containsSuspiciousContent = false;
};

class FolderTrustManager : public QObject {
    Q_OBJECT

public:
    static FolderTrustManager* instance();
    
    // Trust state queries
    bool isFolderTrusted(const QString& folderPath);
    bool containsSuspiciousPatterns(const QString& folderPath);
    FolderTrustInfo getTrustInfo(const QString& folderPath);
    
    // Trust state management
    void markFolderAsTrusted(const QString& folderPath, const QString& reason = "");
    void markFolderAsUntrusted(const QString& folderPath);
    void revokeTrustDecision(const QString& folderPath);
    
    // Trust discovery and scanning
    bool performTrustDiscovery(const QString& folderPath);
    QStringList scanForSuspiciousPatterns(const QString& folderPath);
    QStringList scanForExecutables(const QString& folderPath);
    
    // Trust prompts and UI
    bool shouldPromptForTrust(const QString& folderPath);
    void recordTrustDecision(const QString& folderPath, bool trusted, const QString& reason);
    
    // Trust storage and persistence
    bool saveTrustDecisions();
    bool loadTrustDecisions();
    QString getTrustStoragePath() const;
    void clearTrustData();
    
    // Batch operations
    QStringList getTrustedFolders() const;
    QStringList getUntrustedFolders() const;
    void markMultipleFoldersAsTrusted(const QStringList& folders);
    
    // Monitoring
    bool isMonitoring(const QString& folderPath) const;
    void startMonitoring(const QString& folderPath);
    void stopMonitoring(const QString& folderPath);
    
    // Helper functions
    bool isSystemFolder(const QString& folderPath) const;

signals:
    void folderTrustChanged(const QString& folderPath, bool isTrusted);
    void trustDecisionRequired(const QString& folderPath, const QStringList& suspiciousItems);
    void folderScanned(const QString& folderPath);
    void suspiciousContentFound(const QString& folderPath, const QStringList& items);

private:
    FolderTrustManager();
    ~FolderTrustManager() override;
    
    class Impl;
    std::unique_ptr<Impl> m_impl;
    mutable QRecursiveMutex m_mutex;
};
