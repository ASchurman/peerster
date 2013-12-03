#ifndef CRYPTO_HH
#define CRYPTO_HH

#include <QtCrypto>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QSet>
#include <QVariantMap>

#define RSA_BITS (1024)
#define RSA_EXP (65537)

class Crypto
{
public:
    Crypto();

    // Returns the RSA public key value as an array ready to be sent over the
    // network
    QByteArray pubKeyVal() { return m_pub.toRSA().n().toArray().toByteArray(); }

    // Encrypts the given data with the given user's public key. Returns an
    // empty array if there's no public key on record for the given user
    QByteArray encrypt(const QString& dest, const QByteArray& data);
    QByteArray encrypt(const QString& dest, const QVariantMap& map)
    { return encrypt(dest, serialize(map)); }

    // Decrypts the given data with my private key
    QByteArray decrypt(const QByteArray& data);
    QVariantMap decryptMap(const QByteArray& data)
    { return deserialize(decrypt(data)); }

    // Signs the given data with my private key and returns the signature
    QByteArray sign(const QByteArray& data);
    QByteArray sign(const QVariantMap& map)
    { return sign(serialize(map)); }

    // Checks the validity of another user's signature. Returns true if valid.
    bool checkSig(const QString& origin, const QByteArray& data, const QByteArray& sig);
    bool checkSig(const QString& origin, const QVariantMap& map, const QByteArray& sig)
    { return checkSig(origin, serialize(map), sig); }

    // Checks if the given user is trusted; returns true if trusted
    bool isTrusted(const QString& name);

    // Adds a user's public key to the table
    void addPubKey(const QString& name, const QByteArray& pubKey);

    // Returns a list of users who have signed my public key and thus trust me
    QList<QString> keySigList() { return m_keySigs.keys(); }

    // Returns the signature associated with the given name. If no such
    // key exists, returns an empty array
    QByteArray getKeySig(const QString& name);

    // Add someone's signature of my public key. Returns true if it's a valid
    // signature; else returns false.
    bool addKeySig(const QString& name, const QByteArray& sig);

private:
    // My key pair
    QCA::PrivateKey m_priv;
    QCA::PublicKey m_pub;

    // Table of signatures from other users of my public key.
    QHash<QString, QByteArray> m_keySigs;

    // Table of public keys from other users, keyed by origin name
    QHash<QString, QCA::PublicKey> m_pubTable;

    // Set of trusted origin names
    QSet<QString> m_trusted;

    QByteArray serialize(const QVariantMap& map);
    QVariantMap deserialize(const QByteArray& data);
};

extern Crypto* GlobalCrypto;

#endif // CRYPTO_HH
