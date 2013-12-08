#include "trustchallenge.hh"
#include "crypto.hh"

#include <QDebug>

TrustChallenge::TrustChallenge(const QString& name, const QString& answer)
{
    m_name = name;
    m_key = makeKey(answer);
}

QCA::SymmetricKey TrustChallenge::makeKey(const QString& answer)
{
    QByteArray a = answer.toAscii();
    a.truncate(32);
    while (a.size() < 32) a.append('\0');
    return QCA::SymmetricKey(a);
}

bool TrustChallenge::check(const QByteArray& pubKey,
                           const QByteArray& cryptPubKey)
{
    QCA::Cipher cipher(QString("aes256"), QCA::Cipher::CBC,
                       QCA::Cipher::DefaultPadding,
                       QCA::Decode, m_key);
    QCA::SecureArray decryptedPubKey = cipher.process(QCA::SecureArray(cryptPubKey));
    if (!cipher.ok()
        || decryptedPubKey.toByteArray() != pubKey)
    {
        qDebug() << "User " << m_name << " has FAILED the challenge";
        return false;
    }
    else
    {
        qDebug() << "User " << m_name << " has PASSED the challenge";
        return true;
    }
}
