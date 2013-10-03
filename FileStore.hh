#ifndef FILE_STORE_HH
#define FILE_STORE_HH

#include <QObject>
#include <QString>
#include <QHash>
#include <QByteArray>
#include <QList>

#include "FileData.hh"

// A store for all FileData objects
class FileStore : public QObject
{
    Q_OBJECT

public:
    FileStore() { }

    // Add a file for this node to share
    bool addSharingFile(QString& fileName);

    // Returns the block associated with the given hash.
    bool findBlock(QByteArray& hash, QByteArray& outBlock);

    // Searches files being shared by this host for fileNames containing one
    // of the space-separated search terms in searchTerms. Populates outFileIds
    // with the fileIds of matching files. Returns TRUE if any files match.
    bool findFile(QString& searchTerms,
                  QList<QString>& outFileNames,
                  QList<QByteArray>& outFileIds);

public slots:
    // Add a file to download
    void addDownloadFile(QString& fileName, QByteArray& fileId, QString& host);

    // Adds a block to one of the downloading files. Removes the file from
    // the store of downloading files if this is the last block. Verifies that
    // the blockHash is the correct hash of the data before adding the block.
    void addBlock(QByteArray& blockHash, QByteArray& data);

private:
    // Contains the FileData for each file being shared on the network.
    // Keyed by the file's ID.
    QHash<QByteArray, FileData*> m_sharingFiles;

    // Contains the FileData for each file being downloaded.
    // Keyed by the file's ID.
    QHash<QByteArray, FileData*> m_downloadingFiles;
};

extern FileStore* GlobalFiles;

#endif
