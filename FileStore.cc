#include <QDebug>

#include "FileStore.hh"

FileStore* GlobalFiles;

bool FileStore::addFile(QString& fileName)
{
    FileData newFile(fileName);
    if (!newFile.isInitialized()) return false;

    m_files.insert(newFile.getFileId(), newFile);
    return true;
}
