#include <QtCrypto>
#include <QFile>
#include <QDebug>

#include "FileData.hh"
#include "NetSocket.hh"

#define BLOCKSIZE (8192)
#define SHA_SIZE (32)

FileData::FileData(QString& fileName, QByteArray& fileId, QString& host)
{
    m_isSharing = false;
    m_blocklist.clear();
    m_blockHash.clear();
    m_fileId = fileId;
    m_name = fileName;
    m_size = -1;
    m_host = host;
}

FileData::FileData(QString& fileName)
{
    open(fileName);
}

bool FileData::open(QString& fileName)
{
    // Clear existing data in case this FileData object is being reused.
    m_blocklist.clear();
    m_blockHash.clear();
    m_fileId.clear();
    m_name.clear();
    m_size = 0;

    // Open the file and check for errors
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "ERROR opening file: " << fileName;
        qDebug() << file.errorString();
        return false;
    }

    // Compute the SHA256 hash of each block in the file. Append each
    // hash to m_blocklist.
    char block[BLOCKSIZE];
    qint64 read;
    QCA::Hash shaHash("sha256");
    for(int i = 0; (read = file.read(block, BLOCKSIZE)) > 0; i++)
    {
        // Store the read data
        m_data.append(QByteArray(block, read));

        // Compute the hash of block i and store it in hash
        shaHash.update(block, read);
        QByteArray hash = shaHash.final().toByteArray();
        shaHash.clear();

        // Append hash to m_blocklist and insert it in m_blockHash
        m_blocklist += hash;
        m_blockHash.insert(hash, i);
    }

    // Check for errors in reading from the file
    if (read == -1)
    {
        qDebug() << "ERROR reading file: " << fileName;
        qDebug() << file.errorString();
        return false;
    }

    // Compute the hash of the blocklist and store it in m_fileId
    shaHash.update(m_blocklist);
    m_fileId = shaHash.final().toByteArray();
    shaHash.clear();

    // Populate remaining fields.
    m_size = file.size();
    m_name = fileName;
    m_isSharing = true;

    // Print debug info
    qDebug() << "NAME: " << m_name;
    qDebug() << "FRIENDLY NAME: " << getFriendlyName();
    qDebug() << "SIZE: " << m_size;
    qDebug() << "NUM BLOCKS: " << m_data.count();
    qDebug() << "ID: " << m_fileId.toHex().data();

    return true;
}

bool FileData::containsHash(QByteArray& hash)
{
    return m_fileId == hash || m_blockHash.contains(hash);
}

qint64 FileData::findBlock(QByteArray& hash)
{
    if (!containsHash(hash))
    {
        qDebug() << "        FileData doesn't contain requested hash";
        return -1;
    }
    else if (m_fileId == hash)
    {
        qDebug() << "        hash matches fileId; get it directly instead of calling findBlock";
        return -1;
    }
    else
    {
        qDebug() << "        FileData returning block " << m_blockHash[hash];
        return m_blockHash[hash];
    }
}

void FileData::requestBlock()
{
    if (m_isSharing)
    {
        qDebug() << "Trying to request blocks for a file being shared";
        return;
    }
    else if (fileComplete())
    {
        qDebug() << "Trying to request blocks for a complete file";
        return;
    }

    if (m_blocklist.isEmpty())
    {
        qDebug() << "REQUESTING BLOCKLIST: " << m_name;
        GlobalSocket->requestBlock(m_fileId, m_host);
    }
    else
    {
        qDebug() << "REQUESTING BLOCK for file: " << m_name;

        QList<QByteArray> allHashes = m_blockHash.keys();
        for (qint64 i = 0; i < allHashes.count(); i++)
        {
            if (!m_obtainedBlocks.contains(m_blockHash[allHashes[i]]))
            {
                qDebug() << "    REQUESTING BLOCK " << m_blockHash[allHashes[i]];
                GlobalSocket->requestBlock(allHashes[i], m_host);
                return;
            }
        }
        qDebug() << "    NEED BLOCK, BUT CANNOT FIND NEEDED BLOCK!!!!";
    }
}

bool FileData::addBlock(QByteArray& hash, QByteArray& block)
{
    if (m_isSharing) return false;

    qDebug() << "ADDING BLOCK for file: " << m_name;

    // Verify the hash
    QCA::Hash shaHash("sha256");
    shaHash.update(block);
    if (hash != shaHash.final().toByteArray())
    {
        qDebug() << "    BAD HASH RECEIVED";
        return false;
    }

    // Check if it's the blocklist before checking if it's a normal block
    if (hash == m_fileId)
    {
        qDebug() << "    GOT BLOCKLIST for file: " << m_name;
        m_blocklist = block;

        // Populate blockHash by breaking down blocklist
        QByteArray cpBlocklist = block;
        qint64 i;
        for (i = 0; !cpBlocklist.isEmpty(); i++)
        {
            m_blockHash.insert(cpBlocklist.left(SHA_SIZE), i);
            cpBlocklist.remove(0, SHA_SIZE);

            // Append an empty block to our data for each block in the blocklist
            m_data.append(QByteArray());
        }

        // Request a new block
        requestBlock();
        return false;
    }

    // Check if this is one of our normal blocks. This is a new block if it's
    // in our blocklist but not in our obtained blocks
    if (m_blockHash.contains(hash)
            && !m_obtainedBlocks.contains(m_blockHash[hash]))
    {
        // Add the block to our data and obtained block set
        qDebug() << "    GOT BLOCK: " << m_blockHash[hash];
        m_data[m_blockHash[hash]] = block;
        m_obtainedBlocks.insert(m_blockHash[hash]);

        // Are we done? If so, save the file
        if (fileComplete())
        {
            qDebug() << "    FILE COMPLETE: " << m_name;
            save();
            return true;
        }
        else
        {
            requestBlock();
            return false;
        }
    }
    else if (!m_blockHash.contains(hash))
    {
        qDebug() << "    Nevermind...this file doesn't contain this hash";
        return false;
    }
    else if (m_obtainedBlocks.contains(m_blockHash[hash]))
    {
        qDebug() << "    Nevermind...we already have block " << m_blockHash[hash];
        return false;
    }
    qDebug() << "    Something weird happened....";
    return false;
}

bool FileData::save()
{
    qDebug() << "SAVING FILE: " << m_name;

    QFile file(m_name);
    if (!file.open(QIODevice::ReadWrite))
    {
        qDebug() << "    OPEN FILE FAILED: " << m_name;
        return false;
    }

    for (qint64 i = 0; i < m_data.count(); i++)
    {
        file.write(m_data[i]);
    }
    return true;
}

QString FileData::getFriendlyName()
{
    QStringList strList = m_name.split('/');
    return strList.last();
}
