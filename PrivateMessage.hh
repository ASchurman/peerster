#ifndef PRIVATEMESSAGE_HH
#define PRIVATEMESSAGE_HH

#include <QString>
#include <QByteArray>
#include <QVariantList>

// Abstract superclass for all types of private point-to-point messages
class PrivateMessage
{
public:
    PrivateMessage(QString& dest,
                   int hopLimit)
        : m_dest(dest), m_hopLimit(hopLimit), m_origin()
    { }

    PrivateMessage(QString& dest,
                   int hopLimit,
                   QString& origin)
        : m_dest(dest), m_hopLimit(hopLimit), m_origin(origin)
    { }

    virtual ~PrivateMessage() { }

    enum PrivateType
    {
        Undefined = 0,
        Chat,
        BlockReq,
        BlockRep,
        SearchRep
    };

    virtual PrivateType type() = 0;

    QString m_dest;
    int m_hopLimit;

    // True if the signature on the private message is valid
    bool m_validSig;

    // ORIGIN value which may not be defined for incoming messages
    bool hasOrigin() { return !m_origin.isEmpty(); }
    QString m_origin;
};

// Holds content of a private chat message
class PrivateChat : public PrivateMessage
{
public:
    PrivateChat(QString& dest,
                int hopLimit,
                QString& text)
        : PrivateMessage(dest, hopLimit), m_text(text)
    { }

    PrivateChat(QString& dest,
                int hopLimit,
                QString& text,
                QString& origin)
        : PrivateMessage(dest, hopLimit, origin), m_text(text)
    { }

    PrivateType type() { return Chat; }

    QString m_text;
};

// Abstract superclass for point-to-point messages involved in file-transfers
class PrivateBlockMessage : public PrivateMessage
{
public:
    PrivateBlockMessage(QString& dest,
                        int hopLimit,
                        QByteArray& hash,
                        QString& origin)
        : PrivateMessage(dest, hopLimit, origin), m_hash(hash)
    { }

    virtual ~PrivateBlockMessage() { }

    QByteArray m_hash;
};

// Holds content of a block request message
class PrivateBlockReq : public PrivateBlockMessage
{
public:
    PrivateBlockReq(QString& dest,
                    int hopLimit,
                    QByteArray& hash,
                    QString& origin)
        : PrivateBlockMessage(dest, hopLimit, hash, origin)
    { }

    PrivateType type() { return BlockReq; }
};

// Holds content of a block reply message
class PrivateBlockRep : public PrivateBlockMessage
{
public:
    PrivateBlockRep(QString& dest,
                    int hopLimit,
                    QByteArray& hash,
                    QByteArray& data,
                    QString& origin)
        : PrivateBlockMessage(dest, hopLimit, hash, origin), m_data(data)
    { }

    PrivateType type() { return BlockRep; }

    QByteArray m_data;
};

// Holds content of a search reply message
class PrivateSearchRep : public PrivateMessage
{
public:
    PrivateSearchRep(QString& dest,
                     int hopLimit,
                     QString& searchTerms,
                     QVariantList& resultFileNames,
                     QVariantList& resultHashes,
                     QString& origin)
        : PrivateMessage(dest, hopLimit, origin),
          m_searchTerms(searchTerms),
          m_resultFileNames(resultFileNames),
          m_resultHashes(resultHashes)
    { }

    PrivateType type() { return SearchRep; }

    QString m_searchTerms;
    QVariantList m_resultFileNames;
    QVariantList m_resultHashes;
};

#endif // PRIVATEMESSAGE_HH
