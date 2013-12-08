#ifndef PRIVATEMESSAGE_HH
#define PRIVATEMESSAGE_HH

#include <QString>
#include <QByteArray>
#include <QVariantList>

// Abstract superclass for all types of private point-to-point messages
class PrivateMessage
{
public:
    PrivateMessage(const QString& dest,
                   int hopLimit)
        : m_dest(dest), m_hopLimit(hopLimit), m_origin()
    { }

    PrivateMessage(const QString& dest,
                   int hopLimit,
                   const QString& origin)
        : m_dest(dest), m_hopLimit(hopLimit), m_origin(origin)
    { }

    virtual ~PrivateMessage() { }

    enum PrivateType
    {
        Undefined = 0,
        Chat,
        BlockReq,
        BlockRep,
        SearchRep,
        Challenge,
        ChallengeResponse,
        ChallengeSig,
        SignatureRequest,
        SignatureResponse
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

// Holds content of a trust challenge message
class PrivateChallenge : public PrivateMessage
{
public:
    PrivateChallenge(const QString& dest,
                     int hopLimit,
                     const QString& origin,
                     const QString& challenge)
        : PrivateMessage(dest, hopLimit, origin), m_challenge(challenge)
    { }

    PrivateType type() { return Challenge; }

    QString m_challenge;
};

class PrivateChallengeRep : public PrivateMessage
{
public:
    PrivateChallengeRep(const QString& dest,
                        int hopLimit,
                        const QString& origin,
                        const QByteArray& response)
        : PrivateMessage(dest, hopLimit, origin), m_response(response)
    { }

    PrivateType type() { return ChallengeResponse; }

    QByteArray m_response;
};

class PrivateChallengeSig : public PrivateMessage
{
public:
    PrivateChallengeSig(const QString& dest,
                        int hopLimit,
                        const QString& origin,
                        const QByteArray& sig)
        : PrivateMessage(dest, hopLimit, origin), m_sig(sig)
    { }

    PrivateType type() { return ChallengeSig; }

    QByteArray m_sig;
};

class PrivateSigReq : public PrivateMessage
{
public:
    PrivateSigReq(const QString& dest,
                  int hopLimit,
                  const QString& origin,
                  const QString& name)
        : PrivateMessage(dest, hopLimit, origin), m_name(name)
    { }

    PrivateType type() { return SignatureRequest; }

    // Name of the individual whose signature we're requesting
    QString m_name;
};

class PrivateSigRep : public PrivateMessage
{
public:
    PrivateSigRep(const QString& dest,
                  int hopLimit,
                  const QString& origin,
                  const QString& name,
                  const QByteArray& sig)
        : PrivateMessage(dest, hopLimit, origin), m_name(name), m_sig(sig)
    { }

    PrivateType type() { return SignatureResponse; }

    QString m_name;
    QByteArray m_sig;
};

// Holds content of a private chat message
class PrivateChat : public PrivateMessage
{
public:
    PrivateChat(const QString& dest,
                int hopLimit,
                const QString& text)
        : PrivateMessage(dest, hopLimit), m_text(text)
    { }

    PrivateChat(const QString& dest,
                int hopLimit,
                const QString& text,
                const QString& origin)
        : PrivateMessage(dest, hopLimit, origin), m_text(text)
    { }

    PrivateType type() { return Chat; }

    QString m_text;
};

// Abstract superclass for point-to-point messages involved in file-transfers
class PrivateBlockMessage : public PrivateMessage
{
public:
    PrivateBlockMessage(const QString& dest,
                        int hopLimit,
                        const QByteArray& hash,
                        const QString& origin)
        : PrivateMessage(dest, hopLimit, origin), m_hash(hash)
    { }

    virtual ~PrivateBlockMessage() { }

    QByteArray m_hash;
};

// Holds content of a block request message
class PrivateBlockReq : public PrivateBlockMessage
{
public:
    PrivateBlockReq(const QString& dest,
                    int hopLimit,
                    const QByteArray& hash,
                    const QString& origin)
        : PrivateBlockMessage(dest, hopLimit, hash, origin)
    { }

    PrivateType type() { return BlockReq; }
};

// Holds content of a block reply message
class PrivateBlockRep : public PrivateBlockMessage
{
public:
    PrivateBlockRep(const QString& dest,
                    int hopLimit,
                    const QByteArray& hash,
                    const QByteArray& data,
                    const QString& origin)
        : PrivateBlockMessage(dest, hopLimit, hash, origin), m_data(data)
    { }

    PrivateType type() { return BlockRep; }

    QByteArray m_data;
};

// Holds content of a search reply message
class PrivateSearchRep : public PrivateMessage
{
public:
    PrivateSearchRep(const QString& dest,
                     int hopLimit,
                     const QString& searchTerms,
                     const QVariantList& resultFileNames,
                     const QVariantList& resultHashes,
                     const QString& origin)
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
