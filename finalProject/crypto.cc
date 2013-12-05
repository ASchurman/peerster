#include "crypto.hh"

#include <QDebug>
#include <QDataStream>

Crypto* GlobalCrypto;

QByteArray Crypto::serialize(const QVariantMap& map)
{
    QByteArray data;
    data.resize(sizeof(map));
    QDataStream dataStream(&data, QIODevice::WriteOnly);
    dataStream << map;
    return data;
}

QVariantMap Crypto::deserialize(const QByteArray& data)
{
    QByteArray d(data);
    QVariantMap map;
    QDataStream dataStream(&d, QIODevice::ReadOnly);
    dataStream >> map;
    return map;
}

QList<QVariant> Crypto::keySigList()
{
    QStringList keySigs = m_keySigs.keys();
    QVector<QVariant> v(keySigs.size());
    qCopy(keySigs.begin(), keySigs.end(), v.begin());
    return v.toList();
}

Crypto::Crypto()
{
    // Create my private key
    m_priv = QCA::KeyGenerator().createRSA(RSA_BITS, RSA_EXP);
    if (m_priv.isNull())
    {
        qDebug() << "FAILED TO MAKE PRIVATE RSA KEY";
    }

    // Extract the public component
    m_pub = m_priv.toPublicKey();

    qDebug() << "My public key: " << pubKeyVal().toHex();
}

QByteArray Crypto::encrypt(const QString& dest, const QByteArray& data)
{
    if (!m_pubTable.contains(dest))
    {
        qDebug() << "No public key on record for user " << dest;
        return QByteArray();
    }

    QCA::SecureArray plainText(data);
    QCA::SecureArray result = m_pubTable[dest].encrypt(plainText, QCA::EME_PKCS1_OAEP);
    return result.toByteArray();
}

QByteArray Crypto::decrypt(const QByteArray& data)
{
    QCA::SecureArray cipherText(data);
    QCA::SecureArray plainText;

    if (0 == m_priv.decrypt(cipherText, &plainText, QCA::EME_PKCS1_OAEP))
    {
        qDebug() << "ERROR DECRYPTING";
        return QByteArray();
    }
    else
    {
        return plainText.toByteArray();
    }
}

QByteArray Crypto::sign(const QByteArray& data)
{
    QCA::SecureArray message(data);
    return m_priv.signMessage(message, QCA::EMSA3_MD5);
}

bool Crypto::checkSig(const QString& origin, const QByteArray& data, const QByteArray& sig)
{
    if (!m_pubTable.contains(origin))
    {
        qDebug() << "No pubic key on record for user " << origin;
        return false;
    }

    QCA::SecureArray message(data);
    return m_pubTable[origin].verifyMessage(message, sig, QCA::EMSA3_MD5);
}

QByteArray Crypto::pubKeyVal(const QString& name)
{
    if (m_pubTable.contains(name))
    {
        return m_pubTable[name].toRSA().n().toArray().toByteArray();
    }
    else
    {
        return QByteArray();
    }
}

bool Crypto::isTrusted(const QString& name)
{
    return m_trusted.contains(name);
}

void Crypto::addPubKey(const QString& name, const QByteArray& pubKey)
{
    if (m_pubTable.contains(name))
    {
        if (m_pubTable[name].toRSA().n().toArray().toByteArray() != pubKey)
        {
            qDebug() << "RECEIVED CONFLICTING PUB KEYS FOR USER " << name;
        }
    }
    else
    {
        // Construct RSAPublicKey object from pubKey value
        QCA::SecureArray secPubKey(pubKey);
        QCA::BigInteger n(secPubKey);
        QCA::RSAPublicKey rsaPubKey(n, QCA::BigInteger(RSA_EXP));

        // Insert it into table
        m_pubTable.insert(name, rsaPubKey.toPublicKey());

        qDebug() << "Received new pubkey for user " << name;
    }
}

QByteArray Crypto::getKeySig(const QString& name)
{
    if (m_keySigs.contains(name))
    {
        return m_keySigs[name];
    }
    else
    {
        qDebug() << "Requested key signature from user who doesn't trust me";
        return QByteArray();
    }
}

bool Crypto::addKeySig(const QString& name, const QByteArray& sig)
{
    if (checkSig(name, pubKeyVal(), sig))
    {
        // Valid signature for user name!
        m_keySigs.insert(name, sig);
        return true;
    }
    else
    {
        // Invalid signature or we don't have a public key for user name
        return false;
    }
}

void Crypto::startChallenge(const QString& dest, const QString& answer)
{
    // TODO startChallenge

    // if there's already a challenge started for dest, remove it and start a
    // new one
}

bool Crypto::endChallenge(const QString& dest, const QByteArray& cryptKey)
{
    // TODO endChallenge
    return true;
}

QList<QVariant> Crypto::keySigList(const QString& name)
{
    if (m_keySigLists.contains(name))
    {
        return m_keySigLists[name];
    }
    else
    {
        return QList<QVariant>();
    }
}

void Crypto::updateKeySigList(const QString& name,
                              const QList<QVariant> keySigList)
{
    m_keySigLists.insert(name, keySigList);
}
