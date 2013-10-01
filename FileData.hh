#ifndef FILE_DATA_HH
#define FILE_DATA_HH

#include <QString>
#include <QByteArray>
#include <QHash>

// Contains data for a single file being shared on the network
class FileData
{
public:
    FileData() : m_isInitialized(false) { }
    FileData(QString& fileName);

    // Returns the value of m_isInitialized after attempting to open a file.
    bool open(QString& fileName);

    // Accessors; if isInitialized returns false, there are no guarantees about
    // other values
    bool isInitialized() { return m_isInitialized; }
    QString getFullName() { return m_name; }
    QString getFriendlyName();
    qint64 getSize() { return m_size; }
    QByteArray getBlocklist() { return m_blocklist; }
    QByteArray getFileId() { return m_fileId; }

private:
    bool m_isInitialized;

    // Fully qualified name of the file
    QString m_name;

    // Size of the file in bytes
    qint64 m_size;

    // Concatenation of the 32-byte SHA-256 hashes for each 8 KB block
    QByteArray m_blocklist;

    // Contains the 0-based index of the block for each SHA-256 hash.
    QHash<QByteArray, qint64> m_blockHash;

    // SHA-256 hash of m_blocklist
    QByteArray m_fileId;
};

#endif
