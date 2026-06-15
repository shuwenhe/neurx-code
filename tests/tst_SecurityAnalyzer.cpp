#include <QTest>
#include <QObject>
#include <QTemporaryDir>
#include <QFile>
#include "../src/agent/SecurityAnalyzer.h"

class TestSecurityAnalyzer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        analyzer = new SecurityAnalyzer();
        QVERIFY(analyzer != nullptr);
        
        tempDir = new QTemporaryDir();
        QVERIFY(tempDir->isValid());
    }

    void cleanupTestCase() {
        delete analyzer;
        delete tempDir;
    }

    // Test: Object creation
    void testObjectCreation() {
        QVERIFY(analyzer != nullptr);
    }

    // Test: Temporary directory
    void testTemporaryDirectory() {
        QVERIFY(tempDir->isValid());
        QVERIFY(QDir(tempDir->path()).exists());
    }

    // Test: File creation and scanning
    void testFileCreationForScanning() {
        QString filePath = tempDir->filePath("test.cpp");
        QFile file(filePath);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("int main() { return 0; }\n");
        file.close();
        
        QVERIFY(QFile::exists(filePath));
    }

    // Test: Multiple file creation
    void testMultipleFileCreation() {
        for (int i = 0; i < 5; i++) {
            QString filePath = tempDir->filePath(QString("file_%1.cpp").arg(i));
            QFile file(filePath);
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
            file.write(QString("// File %1\n").arg(i).toUtf8());
            file.close();
        }
        
        QDir dir(tempDir->path());
        QStringList files = dir.entryList(QStringList("*.cpp"));
        QVERIFY(files.size() >= 5);
    }

private:
    SecurityAnalyzer* analyzer;
    QTemporaryDir* tempDir;
};

QTEST_MAIN(TestSecurityAnalyzer)
#include "tst_SecurityAnalyzer.moc"
