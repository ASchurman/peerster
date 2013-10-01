#include <QtCrypto>
#include <QFile>
#include <QDebug>

#include "FileData.hh"

#define BLOCKSIZE (8192)

FileData::FileData(QString& fileName)
{
    m_isInitialized = false;
    open(fileName);
}

bool FileData::open(QString& fileName)
{
    // Clear existing data in case this FileData object is being reused.
    if (m_isInitialized) qDebug() << "REUSING FILE DATA OBJECT";
    m_isInitialized = false;
    m_blocklist.clear();
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
    m_isInitialized = true;

    // Print debug info
    qDebug() << "NAME: " << m_name;
    qDebug() << "FRIENDLY NAME: " << getFriendlyName();
    qDebug() << "SIZE : " << m_size;
    qDebug() << "ID: " << m_fileId.toHex().data();

    return true;
}

QString FileData::getFriendlyName()
{
    if (!m_isInitialized) return "";

    QStringList strList = m_name.split('/');
    return strList.last();
}
