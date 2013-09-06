#include <unistd.h>

#include <QDebug>

#include "NetSocket.hh"
#include "MessageStore.hh"

NetSocket* GlobalSocket;

#define CHAT_TEXT "ChatText"
#define ORIGIN "Origin"
#define SEQ_NO "SeqNo"
#define WANT "Want"

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

    m_seqNo = 1;
    m_hostName.setNum(rand());

    connect(this, SIGNAL(readyRead()), this, SLOT(gotReadyRead()));
}

bool NetSocket::bind()
{
    // Try to bind to each of the range m_myPortMin..m_myPortMax in turn.
    for (int p = m_myPortMin; p <= m_myPortMax; p++)
    {
        if (QUdpSocket::bind(p))
        {
            m_myPort = p;
            qDebug() << "bound to UDP port " << m_myPort;

            if (p - 1 >= m_myPortMin)
            {
                m_neighbors.append(AddrInfo(QHostAddress(QHostAddress::LocalHost),
                                            p - 1));
            }
            if (p + 1 <= m_myPortMax)
            {
                m_neighbors.append(AddrInfo(QHostAddress(QHostAddress::LocalHost),
                                            p + 1));
            }

            return true;
        }
    }

    qDebug() << "Oops, no ports in my default range " << m_myPortMin
        << "-" << m_myPortMax << " available";
    return false;
}

Monger* NetSocket::findSession(AddrInfo addrInfo)
{
    for (int i = 0; i < m_sessionAddrs.count(); i++)
    {
        if (m_sessionAddrs[i] == addrInfo)
        {
            return m_sessionMongers[i];
        }
    }
    return NULL;
}

void NetSocket::gotReadyRead()
{
    // TODO: How do we know if deserialization fails?

    qint64 datagramSize = pendingDatagramSize();

    if (datagramSize > 0)
    {
        QByteArray datagram;
        QVariantMap varMap;
        QHostAddress address;
        int port;

        datagram.resize(datagramSize);
        readDatagram(datagram.data(), datagramSize, &address, (quint16*)&port);

        AddrInfo addrInfo(address, port);

        QDataStream dataStream(&datagram, QIODevice::ReadOnly);
        dataStream >> varMap;

        // varMap now contains the QVariantMap serialized in the received
        // datagram

        if (varMap.count() == 3
            && varMap.contains(CHAT_TEXT)
            && varMap.contains(ORIGIN)
            && varMap.contains(SEQ_NO))
        {
            // received a rumor message!
            MessageInfo mesInf(varMap[CHAT_TEXT].toString(),
                               varMap[ORIGIN].toString(),
                               varMap[SEQ_NO].toInt());

            if (!findSession(addrInfo))
            {
                m_sessionAddrs.append(addrInfo);
                m_sessionMongers.append(new Monger(addrInfo));
            }
            findSession(addrInfo)->receiveMessage(mesInf);
        }
        else if (varMap.count() == 1
                 && varMap.contains(WANT))
        {
            // received a status message!
            QVariantMap remoteStatus(varMap[WANT].toMap());

            if (!findSession(addrInfo))
            {
                m_sessionAddrs.append(addrInfo);
                m_sessionMongers.append(new Monger(addrInfo));
            }
            findSession(addrInfo)->receiveStatus(remoteStatus);
        }
    }
}

void NetSocket::inputMessage(QString& message)
{
    MessageInfo mesInf;
    mesInf.m_body = message;
    mesInf.m_host = m_hostName;
    mesInf.m_seqNo = m_seqNo;

    m_seqNo++;

    GlobalMessages->recordMessage(mesInf);
    sendToRandNeighbor(mesInf);
}

void NetSocket::sendToRandNeighbor(MessageInfo& mesInf)
{
    int i = rand() % m_neighbors.count();
    AddrInfo addrInfo = m_neighbors[i];

    sendMessage(mesInf, addrInfo.m_addr, addrInfo.m_port);
}

void NetSocket::sendStatusToRandNeighbor()
{
    int i = rand() % m_neighbors.count();
    AddrInfo addrInfo = m_neighbors[i];

    sendStatus(addrInfo.m_addr, addrInfo.m_port);
}

void NetSocket::sendMessage(MessageInfo& mesInf,
                            QHostAddress address, int port)
{
    QVariantMap varMap;
    varMap.insert(CHAT_TEXT, mesInf.m_body);
    varMap.insert(ORIGIN, mesInf.m_host);
    varMap.insert(SEQ_NO, mesInf.m_seqNo);
    sendMap(varMap, address, port);

    AddrInfo addrInfo(address, port);
    if (!findSession(addrInfo))
    {
        m_sessionAddrs.append(addrInfo);
        m_sessionMongers.append(new Monger(addrInfo));
    }
    findSession(addrInfo)->startTimer();
}

void NetSocket::sendStatus(QHostAddress address, int port)
{
    QVariantMap* status = GlobalMessages->getStatus();
    QVariantMap statusMessage;
    statusMessage.insert(WANT, *status);
    delete status;

    sendMap(statusMessage, address, port);
}

void NetSocket::sendMap(QVariantMap& varMap, QHostAddress address, int port)
{
    QByteArray datagram;
    datagram.resize(sizeof(varMap));
    QDataStream dataStream(&datagram, QIODevice::WriteOnly);
    dataStream << varMap;

    writeDatagram(datagram, address, port);
}
