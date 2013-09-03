#include <unistd.h>

#include <QDebug>

#include "NetSocket.hh"

NetSocket* GlobalSocket;

#define CHAT_TEXT "ChatText"

NetSocket::NetSocket()
{
    // Pick a range of four UDP ports to try to allocate by default,
    // computed based on my Unix user ID.
    // This makes it trivial for up to four Peerster instances per user
    // to find each other on the same host,
    // barring UDP port conflicts with other applications
    // (which are quite possible).
    // We use the range from 32768 to 49151 for this purpose.
    m_myPortMin = 32768 + (getuid() % 4096)*4;
    m_myPortMax = m_myPortMin + 3;

    connect(this, SIGNAL(readyRead()), this, SLOT(gotReadyRead()));
}

bool NetSocket::bind()
{
    // Try to bind to each of the range m_myPortMin..m_myPortMax in turn.
    for (int p = m_myPortMin; p <= m_myPortMax; p++)
    {
        if (QUdpSocket::bind(p))
        {
            qDebug() << "bound to UDP port " << p;
            return true;
        }
    }

    qDebug() << "Oops, no ports in my default range " << m_myPortMin
        << "-" << m_myPortMax << " available";
    return false;
}

void NetSocket::send(QString& message)
{
    QVariantMap varMap;
    varMap.insert(CHAT_TEXT, message);

    QByteArray datagram;
    datagram.resize(sizeof(varMap));
    QDataStream dataStream(&datagram, QIODevice::WriteOnly);
    dataStream << varMap;

    for (int p = m_myPortMin; p <= m_myPortMax; p++)
    {
        writeDatagram(datagram, QHostAddress(QHostAddress::LocalHost), p);
    }
}

void NetSocket::gotReadyRead()
{
    // TODO: How do we know if deserialization fails? In that case, we should
    // return NULL before allocating a QString.

    qint64 datagramSize = pendingDatagramSize();

    if (datagramSize > 0)
    {
        QByteArray datagram;
        QVariantMap varMap;

        datagram.resize(datagramSize);
        readDatagram(datagram.data(), datagramSize);

        QDataStream dataStream(&datagram, QIODevice::ReadOnly);
        dataStream >> varMap;

        if (varMap.contains(CHAT_TEXT))
        {
            QString* pMessage = new QString(varMap[CHAT_TEXT].toString());
            emit messageReceived(*pMessage);
        }
    }
}
