#ifndef TRUSTCHALLENGE_HH
#define TRUSTCHALLENGE_HH

#include <QtCrypto>
#include <QString>

// Contains state for a single trust challenge initiated by me
class TrustChallenge
{
public:
    TrustChallenge() { }
    TrustChallenge(const QString& name, const QString& answer);

    // Constructs an AES256 key from the answer to a challenge
    static QCA::SymmetricKey makeKey(const QString& answer);

    // Verifies the respose to a challenge. pubKey is the public key that we
    // have on record for this user, and cryptPubKey is the encrypted copy of
    // his key that he send as a response to the challenge.
    // Decrypts cryptPubKey with m_key, and if it equals pubKey, then the user
    // passes.
    // Returns true if the user passes the challenge.
    bool check(const QByteArray& pubKey, const QByteArray& cryptPubKey);

    // User name that we're challenging
    QString m_name;

    // The AES256 key derived from the answer to the challenge
    QCA::SymmetricKey m_key;
};

#endif // TRUSTCHALLENGE_HH
