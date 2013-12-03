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
    // TODO addKeySig
}
