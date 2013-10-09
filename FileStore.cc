#include <QDebug>
#include <QStringList>

#include "FileStore.hh"

FileStore* GlobalFiles;

bool FileStore::addSharingFile(QString& fileName)
{
    FileData* newFile = new FileData(fileName);

    m_sharingFiles.insert(newFile->m_fileId, newFile);
    return true;
}

bool FileStore::findBlock(QByteArray& hash, QByteArray& outBlock)
{
    if (m_sharingFiles.contains(hash))
    {
        // We were given a fileID, so return the blocklist
        qDebug() << "    requested blocklist for " << m_sharingFiles[hash]->m_name;
        outBlock = m_sharingFiles[hash]->m_blocklist;
        return true;
    }
    else
    {
        // Check each file to see if it it contains the hash
        QList<FileData*> files = m_sharingFiles.values();
        for (int i = 0; i < files.count(); i++)
        {
            if (files[i]->containsHash(hash))
            {
                qDebug() << "    found file with requested block: " << files[i]->m_name;
                qint64 blockNo = files[i]->findBlock(hash);
                if (blockNo == -1)
                {
                    qDebug() << "    BUG...File doesn't contain block after all;";
                    return false;
                }
                outBlock = files[i]->m_data[blockNo];
                return true;
            }
        }
        qDebug() << "    requested hash doesn't match any of our blocks";
        return false;
    }
}

void FileStore::addDownloadFile(QString& fileName, QByteArray &fileId, QString& host)
{
    FileData* newFile = new FileData(fileName, fileId, host);

    m_downloadingFiles.insert(fileId, newFile);
    m_downloadingFiles[fileId]->requestBlock();
}

void FileStore::addBlock(QByteArray &blockHash, QByteArray &data)
{
    QList<QByteArray> downIds = m_downloadingFiles.keys();

    for (int i = 0; i < downIds.count(); i++)
    {
        if (m_downloadingFiles[downIds[i]]->containsHash(blockHash))
        {
            // Add the block to the file and remove it from pending files
            // if it's done downloading
            if(m_downloadingFiles[downIds[i]]->addBlock(blockHash, data))
            {
                // TODO reshare completed files?
                FileData* doneFile = m_downloadingFiles[downIds[i]];
                m_downloadingFiles.remove(downIds[i]);
                delete doneFile;
                return;
            }
        }
    }
}

bool FileStore::findFile(QString &searchTerms,
                         QList<QString> &outFileNames,
                         QList<QByteArray> &outFileIds)
{
    QStringList terms = searchTerms.split(" ", QString::SkipEmptyParts);
    bool found = false;

    QList<FileData*> files = m_sharingFiles.values();
    for (int i = 0; i < files.count(); i++)
    {
        QString friendlyName = files[i]->getFriendlyName();
        for (int j = 0; j < terms.count(); j++)
        {
            if (friendlyName.contains(terms[j], Qt::CaseInsensitive))
            {
                found = true;
                outFileNames.append(friendlyName);
                outFileIds.append(files[i]->m_fileId);
                break;
            }
        }
    }
    return found;
}
