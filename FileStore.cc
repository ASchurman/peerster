#include <QDebug>

#include "FileStore.hh"

FileStore* GlobalFiles;

bool FileStore::addSharingFile(QString& fileName)
{
    FileData newFile(fileName);

    m_sharingFiles.insert(newFile.m_fileId, newFile);
    return true;
}

bool FileStore::findBlock(QByteArray& hash, QByteArray& outBlock)
{
    if (m_sharingFiles.contains(hash))
    {
        // We were given a fileID, so return the blocklist
        qDebug() << "    requested blocklist for " << m_sharingFiles[hash].m_name;
        outBlock = m_sharingFiles[hash].m_blocklist;
        return true;
    }
    else
    {
        // Check each file to see if it it contains the hash
        QList<FileData> files = m_sharingFiles.values();
        for (int i = 0; i < files.count(); i++)
        {
            if (files[i].containsHash(hash))
            {
                qDebug() << "    found file with requested block: " << files[i].m_name;
                qint64 blockNo = files[i].findBlock(hash);
                if (blockNo == -1)
                {
                    qDebug() << "    BUG...File doesn't contain block after all;";
                    return false;
                }
                outBlock = files[i].m_data[blockNo];
                return true;
            }
        }
        qDebug() << "    requested hash doesn't match any of our blocks";
        return false;
    }
}

void FileStore::addDownloadFile(QString& fileName, QByteArray &fileId, QString& host)
{
    m_downloadingFiles.insert(fileId, FileData(fileName, fileId, host));
    m_downloadingFiles[fileId].requestBlock();
}

void FileStore::addBlock(QByteArray &blockHash, QByteArray &data)
{
    QList<QByteArray> downIds = m_downloadingFiles.keys();

    for (int i = 0; i < downIds.count(); i++)
    {
        if (m_downloadingFiles[downIds[i]].containsHash(blockHash))
        {
            // Add the block to the file and remove it from pending files
            // if it's done downloading
            if(m_downloadingFiles[downIds[i]].addBlock(blockHash, data))
            {
                m_downloadingFiles.remove(downIds[i]);
                return;
            }
        }
    }
}
