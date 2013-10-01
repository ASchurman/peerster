#ifndef FILE_STORE_HH
#define FILE_STORE_HH

#include <QObject>
#include <QString>
#include <QHash>
#include <QByteArray>

#include "FileData.hh"

// A store for all FileData objects
class FileStore : public QObject
{
    Q_OBJECT

public:
    FileStore() { }

    bool addFile(QString& fileName);

public slots:
    // Add a new file to the store.
    void addFileSlot(QString& fileName) { addFile(fileName); }

private:
    // Contains the FileData for each file being shared on the network.
    // Keyed by the file's ID.
    QHash<QByteArray, FileData> m_files;
};

extern FileStore* GlobalFiles;

#endif
