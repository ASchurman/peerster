#ifndef CRYPTO_HH
#define CRYPTO_HH

#include <QtCrypto>
#include <QByteArray>
#include <QHash>
#include <QString>
#include <QSet>
#include <QVariantMap>

#include "trustchallenge.hh"

#define RSA_BITS (1024)
#define RSA_EXP (65537)

// Handles cryptographic operations in peerster
class Crypto
{
public:
    Crypto();

    // BASIC CRYPTO OPERATIONS /////////////////////////////////////////////////
    // The following members are basic cryptographic operations like encryption,
    // decryption, and signing.
    ////////////////////////////////////////////////////////////////////////////

    // Encrypts the given data with an ephemeral AES256 key and encrypts the AES
    // key with the given user's public key. Places the encrypted AES key in
    // the argument cryptKey and returns the encrypted data. Returns an empty
    // array if there's no public key on record for the given user.
    QByteArray encrypt(const QString& dest,
                       const QByteArray& data,
                       QByteArray* cryptKey);
    QByteArray encrypt(const QString& dest,
                       const QVariantMap& map,
                       QByteArray* cryptKey)
    { return encrypt(dest, serialize(map), cryptKey); }

    // Decrypts the given encrypted AES256 key with my private key, then uses
    // it to decrypt the given data. Returns the decrypted data.
    QByteArray decrypt(const QByteArray& data, const QByteArray& cryptKey);
    QVariantMap decryptMap(const QByteArray& data, const QByteArray& cryptKey)
    { return deserialize(decrypt(data, cryptKey)); }

    // Signs the given data with my private key and returns the signature
    QByteArray sign(const QByteArray& data);
    QByteArray sign(const QVariantMap& map)
    { return sign(serialize(map)); }

    // Checks the validity of another user's signature. Returns true if valid.
    bool checkSig(const QString& origin, const QByteArray& data, const QByteArray& sig);
    bool checkSig(const QString& origin, const QVariantMap& map, const QByteArray& sig)
    { return checkSig(origin, serialize(map), sig); }

    // Adds a user's public key to the table
    void addPubKey(const QString& name, const QByteArray& pubKey);

    // Returns the RSA public key value as an array ready to be sent over the
    // network
    QByteArray pubKeyVal() { return m_pub.toRSA().n().toArray().toByteArray(); }
    // Returns an empty array if we don't have a public key for name
    QByteArray pubKeyVal(const QString& name);

    // Sets flags to intentionally create invalid signatures and/or invalid
    // encryption for testing purposes
    void setBadSig() { m_badSig = true; }
    void setBadCrypto() { m_badCrypto = true; }

private:
    // My key pair
    QCA::PrivateKey m_priv;
    QCA::PublicKey m_pub;

    // Table of public keys from other users, keyed by origin name
    QHash<QString, QCA::PublicKey> m_pubTable;

    QByteArray serialize(const QVariantMap& map);
    QVariantMap deserialize(const QByteArray& data);

    // Flags for intentionally creating invalid signatures and/or invalid
    // encryption for testing purposes
    bool m_badCrypto;
    bool m_badSig;


    // TRUST CHALLENGE MEMBERS /////////////////////////////////////////////////
    // The following members of Crypto are related to establishing and checking
    // the status of trust between me and other users
    ////////////////////////////////////////////////////////////////////////////
public:
    // Add someone's signature of my public key. Returns true if it's a valid
    // signature; else returns false.
    bool addKeySig(const QString& name, const QByteArray& sig);

    // Signs a user's key and returns the signature
    QByteArray signKey(const QString& name) { return sign(pubKeyVal(name)); }

    // Encrypts my public key with an AES key derived from chalAnswer
    QByteArray encryptKey(const QString& chalAnswer);

    // Returns the signature associated with the given name. If no such
    // key exists, returns an empty array
    QByteArray getKeySig(const QString& name);

    // Returns a list of users who have signed my public key and thus trust me
    QList<QVariant> keySigList();

    // Returns a list of signers of a given individual's public key
    QList<QVariant> keySigList(const QString& name);

    // Updates the list of signers of a given individual's public key
    void updateKeySigList(const QString& name, const QList<QVariant> keySigList);

    // Starts a trust challenge with the given user.
    void startChallenge(const QString& dest, const QString& answer);

    // Verify the reponse for a trust challenge. Returns true if it passes.
    bool endChallenge(const QString& dest, const QByteArray& cryptKey);

    // Checks if the given user is trusted; returns true if trusted
    bool isTrusted(const QString& name);

    // Verifies that sig is signer's signature of name's public key, and that
    // I trust signer. If so, trust name and return true.
    bool addTrust(const QString& name, const QString& signer, const QByteArray& sig);

private:
    // Set of trusted origin names
    QSet<QString> m_trusted;

    // Contains the currently running trust challenges
    QHash<QString, TrustChallenge> m_challenges;

    // Table of signatures from other users of my public key.
    QHash<QString, QByteArray> m_keySigs;

    // Table of lists of key signers, keyed by host name
    QHash<QString, QList<QVariant> > m_keySigLists;
};

extern Crypto* GlobalCrypto;

#endif // CRYPTO_HH
