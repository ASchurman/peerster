#ifndef FILE_DATA_HH
#define FILE_DATA_HH

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QSet>
#include <QTimer>

// Contains data for a single file being shared on the network
class FileData : public QObject
{
    Q_OBJECT

public:
    FileData() { }

    // Create FileData for file we're downloading. Saves the file as fileName
    // when we're done downloading it.
    FileData(QString& fileName, QByteArray& fileId, QString& host);

    // Opens a file on this node
    FileData(QString& fileName);

    // Functions for sharing -----------------------------------------------

    // Returns true if successful; false if failed
    bool open(QString& fileName);

    // Functions for downloading -------------------------------------------

    // Returns true if this file's blocklist contains the given hash
    bool containsHash(QByteArray& hash);

    // Returns index of block with given hash. Returns -1 if no such
    // block exists.
    qint64 findBlock(QByteArray& hash);

    // Adds a downloaded block. Verifies the hash before adding it.
    // Saves the file and returns TRUE if the file's complete.
    bool addBlock(QByteArray& hash, QByteArray& block);

    void requestBlock();

    bool fileComplete()
    {
        return !m_blocklist.isEmpty()
                && m_blockHash.count() == m_obtainedBlocks.count();
    }

    // True if this is a file we're sharing on the network.
    // False if this is a file we're downloading.
    bool m_isSharing;

    // Host that owns the file we're downloading
    QString m_host;

    // Fully qualified name of the file
    QString m_name;

    QString getFriendlyName();

    // Size of the file in bytes
    qint64 m_size;

    // File content. Each element is a block. File is reconstructed by
    // appending the blocks together.
    QList<QByteArray> m_data;

    // Block indicies that we have already obtained. When the size of this
    // equals the size of m_blockHash, the file is done downloading.
    QSet<qint64> m_obtainedBlocks;

    // Concatenation of the 32-byte SHA-256 hashes for each 8 KB block
    QByteArray m_blocklist;

    // Contains the 0-based index of the block for each SHA-256 hash.
    // Just the hashes of the blocklist stored for easy searching.
    QHash<QByteArray, qint64> m_blockHash;

    // SHA-256 hash of m_blocklist
    QByteArray m_fileId;

public slots:
    void timeout();

private:
    // Saves the downloaded file. True if successful.
    bool save();

    void setupTimer();

    QTimer* m_pTimer;
    int m_timeouts;
};

#endif
