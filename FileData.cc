#include <QtCrypto>
#include <QFile>
#include <QDebug>

#include "FileData.hh"
#include "NetSocket.hh"

#define BLOCKSIZE (8192)
#define SHA_SIZE (32)
#define MAX_TIMEOUTS (10)

FileData::FileData(QString& fileName, QByteArray& fileId, QString& host)
{
    m_isSharing = false;
    m_blocklist.clear();
    m_remHashes.clear();
    m_fileId = fileId;
    m_name = fileName;
    m_size = -1;
    m_host = host;
    setupTimer();
}

FileData::FileData(QString& fileName)
{
    open(fileName);
    setupTimer();
}

void FileData::setupTimer()
{
    m_timeouts = 0;
    m_pTimer = new QTimer(this);
    m_pTimer->setInterval(2000);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(timeout()));
}

bool FileData::open(QString& fileName)
{
    // Clear existing data in case this FileData object is being reused.
    m_blocklist.clear();
    m_remHashes.clear();
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

        // Append hash to m_blocklist
        m_blocklist += hash;
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
    if (m_fileId == hash) return true;

    for (qint64 i = 0; i < m_blocklist.size(); i += SHA_SIZE)
    {
        if (m_blocklist.mid(i, SHA_SIZE) == hash) return true;
    }
    return false;
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
        int blockNo;
        for (blockNo = 0; blockNo*SHA_SIZE < m_blocklist.size(); blockNo++)
        {
            if (m_blocklist.mid(blockNo*SHA_SIZE, SHA_SIZE) == hash) break;
        }
        if (blockNo * SHA_SIZE >= m_blocklist.size())
        {
            qDebug() << "        Something went wrong in FileData::findBlock...";
        }
        qDebug() << "        FileData returning block " << blockNo;
        return blockNo;
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
        m_timeouts = 0;
        m_pTimer->start();
    }
    else
    {
        qDebug() << "REQUESTING BLOCK for file: " << m_name;
        if (m_remHashes.isEmpty())
        {
            qDebug() << "    NEED BLOCK, BUT CANNOT FIND NEEDED BLOCK!!!!";
        }

        GlobalSocket->requestBlock(m_remHashes[0], m_host);
        m_timeouts = 0;
        m_pTimer->start();
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
        m_pTimer->stop();
        qDebug() << "    GOT BLOCKLIST for file: " << m_name;
        m_blocklist = block;

        // Populate blockHash by breaking down blocklist
        QByteArray cpBlocklist = block;
        qint64 i;
        for (i = 0; !cpBlocklist.isEmpty(); i++)
        {
            m_remHashes.append(cpBlocklist.left(SHA_SIZE));
            cpBlocklist.remove(0, SHA_SIZE);
        }

        // Request a new block
        requestBlock();
        return false;
    }

    // Check if this is one of our normal blocks. If this equals the head of
    // m_remHashes,then it's a block we requested
    if (!m_remHashes.isEmpty() && m_remHashes[0] == hash)
    {
        m_pTimer->stop();

        // Add the block to our data and obtained block set
        qDebug() << "    GOT BLOCK";
        m_data.append(block);
        m_remHashes.removeAt(0);

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

void FileData::timeout()
{
    m_pTimer->stop();
    m_timeouts++;

    if (m_timeouts > MAX_TIMEOUTS)
    {
        qDebug() << "Giving up on downloading file: " << m_name;
    }
    else if (m_isSharing)
    {
        qDebug() << "Timeout on a FileData being shared: " << m_name;
    }
    else
    {
        qDebug() << "TIMEOUT. Resending a request: " << m_name;
        requestBlock();
    }
}
