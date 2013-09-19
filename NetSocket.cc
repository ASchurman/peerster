#include <unistd.h>

#include <QDebug>

#include "NetSocket.hh"
#include "MessageStore.hh"
#include "RouteTable.hh"
#include "ChatDialog.hh"

NetSocket* GlobalSocket;

#define CHAT_TEXT "ChatText"
#define ORIGIN "Origin"
#define SEQ_NO "SeqNo"
#define WANT "Want"
#define DEST "Dest"
#define HOP_LIMIT "HopLimit"

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
    m_hostName.prepend("Alex");

    connect(this, SIGNAL(readyRead()), this, SLOT(gotReadyRead()));

    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, SIGNAL(timeout()), this, SLOT(sendStatusToRandNeighbor()));
    m_statusTimer->start(10000);

    m_routeTimer = new QTimer(this);
    connect(m_routeTimer, SIGNAL(timeout()), this, SLOT(sendRandRouteRumor()));
    m_routeTimer->start(60000);
}

bool NetSocket::bind()
{
    bool bound = false;

    // Try to bind to each of the range m_myPortMin..m_myPortMax in turn.
    for (int p = m_myPortMin; p <= m_myPortMax; p++)
    {
        if (!bound && QUdpSocket::bind(p))
        {
            m_myPort = p;
            qDebug() << "bound to UDP port " << m_myPort;
            bound = true;
        }
        else
        {
            addNeighbor(AddrInfo(QHostAddress(QHostAddress::LocalHost), p));
        }
    }

    if (bound)
    {
        return true;
    }
    else
    {
        qDebug() << "Oops, no ports in my default range " << m_myPortMin
            << "-" << m_myPortMax << " available";
        return false;
    }
}

Monger* NetSocket::findNeighbor(AddrInfo addrInfo)
{
    for (int i = 0; i < m_neighborAddrs.count(); i++)
    {
        if (m_neighborAddrs[i] == addrInfo)
        {
            return m_neighbors[i];
        }
    }
    return NULL;
}

void NetSocket::addNeighbor(AddrInfo addrInfo)
{
    if (addrInfo.m_isDns)
    {
        m_pendingAddrs.append(addrInfo);
        QHostInfo::lookupHost(addrInfo.m_dns, this,
                              SLOT(lookedUpDns(QHostInfo)));

    }
    else
    {
        m_neighborAddrs.append(addrInfo);
        m_neighbors.append(new Monger(addrInfo));

        qDebug() << "Added Neighbor " << addrInfo.m_addr.toString() << ":"
            << addrInfo.m_port;
    }
}

void NetSocket::addNeighbor(QString& hostPortStr)
{
    if (hostPortStr.count(':') == 1)
    {
        QString portStr = hostPortStr.section(':', 1, 1);
        int port = portStr.toInt();

        // port 0 is reserved and should not be used for UDP traffic, so if
        // we see port 0, we can assume that portStr failed to be parsed as an
        // int.
        if (port == 0) return;

        QString hostStr = hostPortStr.section(':', 0, 0);

        QHostAddress addr(hostStr);
        if (addr == QHostAddress::Null)
        {
            // user provided dns name
            addNeighbor(AddrInfo(hostStr, port));
        }
        else
        {
            // user provided IP address
            addNeighbor(AddrInfo(addr, port));
        }
    }
}

void NetSocket::lookedUpDns(const QHostInfo& host)
{
    bool success = host.error() == QHostInfo::NoError;

    if (!success)
    {
        qDebug() << "Lookup failed:" << host.errorString();
    }

    for (int i = 0; i < m_pendingAddrs.count(); i++)
    {
        if (m_pendingAddrs[i].m_dns == host.hostName())
        {
            if (success)
            {
                addNeighbor(AddrInfo(host.addresses().first(),
                                     m_pendingAddrs[i].m_port));

                qDebug() << "Added Neighbor " << host.hostName() << ":"
                    << m_pendingAddrs[i].m_port;
            }
            m_pendingAddrs.removeAt(i);
        }
    }
}

void NetSocket::gotReadyRead()
{
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

        if (((varMap.count() == 3 && varMap.contains(CHAT_TEXT)) || varMap.count() == 2)
            && varMap.contains(ORIGIN)
            && varMap.contains(SEQ_NO))
        {
            // received a rumor message!
            MessageInfo mesInf(varMap[ORIGIN].toString(),
                               varMap[SEQ_NO].toInt());

            // add message body if this isn't a route rumor message
            if (varMap.contains(CHAT_TEXT))
            {
                mesInf.addBody(varMap[CHAT_TEXT].toString());
            }

            if (!findNeighbor(addrInfo))
            {
                addNeighbor(addrInfo);
            }
            findNeighbor(addrInfo)->receiveMessage(mesInf, addrInfo);
        }
        else if ((varMap.count() == 3 || varMap.count() == 4)
                 && varMap.contains(DEST)
                 && varMap.contains(HOP_LIMIT)
                 && varMap.contains(CHAT_TEXT))
        {
            // received private message!
            QString chatText(varMap[CHAT_TEXT].toString());
            QString dest(varMap[DEST].toString());
            int hopLimit = varMap[HOP_LIMIT].toInt();

            if (dest == m_hostName)
            {
                // this private message is meant for me
                GlobalChatDialog->printPrivate(chatText);
            }
            else if (hopLimit - 1 > 0)
            {
                sendPrivate(dest, hopLimit - 1, chatText);
            }
        }
        else if (varMap.count() == 1
                 && varMap.contains(WANT))
        {
            // received a status message!
            QVariantMap remoteStatus(varMap[WANT].toMap());

            if (!findNeighbor(addrInfo))
            {
                addNeighbor(addrInfo);
            }
            findNeighbor(addrInfo)->receiveStatus(remoteStatus);
        }
    }
}

void NetSocket::inputMessage(QString& message)
{
    MessageInfo mesInf;
    mesInf.m_isRoute = false;
    mesInf.m_body = message;
    mesInf.m_host = m_hostName;
    mesInf.m_seqNo = m_seqNo;

    m_seqNo++;

    AddrInfo addr(QHostAddress(QHostAddress::LocalHost), m_myPort);

    GlobalMessages->recordMessage(mesInf, addr);
    sendToRandNeighbor(mesInf);
}

void NetSocket::sendToRandNeighbor(MessageInfo& mesInf)
{
    int i = rand() % m_neighborAddrs.count();
    AddrInfo addrInfo = m_neighborAddrs[i];

    sendMessage(mesInf, addrInfo.m_addr, addrInfo.m_port);
}

void NetSocket::sendStatusToRandNeighbor()
{
    int i = rand() % m_neighborAddrs.count();
    AddrInfo addrInfo = m_neighborAddrs[i];

    sendStatus(addrInfo.m_addr, addrInfo.m_port);
}

void NetSocket::sendMessage(MessageInfo& mesInf,
                            QHostAddress address, int port)
{
    QVariantMap varMap;
    if (!mesInf.m_isRoute) varMap.insert(CHAT_TEXT, mesInf.m_body);
    varMap.insert(ORIGIN, mesInf.m_host);
    varMap.insert(SEQ_NO, mesInf.m_seqNo);
    sendMap(varMap, address, port);

    AddrInfo addrInfo(address, port);
    if (!findNeighbor(addrInfo))
    {
        addNeighbor(addrInfo);
    }
    findNeighbor(addrInfo)->m_lastSent = mesInf;
    findNeighbor(addrInfo)->startTimer();
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

void NetSocket::sendMap(QVariantMap& varMap, AddrInfo& addr)
{
    if (!addr.m_isDns)
    {
        sendMap(varMap, addr.m_addr, addr.m_port);
    }
    else
    {
        qDebug() << "Attempting to send map to AddrInfo with m_isDns";
    }
}

void NetSocket::sendPrivate(QString& dest, int hopLimit, QString& chatText)
{
    AddrInfo addr;

    if (GlobalRoutes->getNextHop(dest, addr))
    {
        QVariantMap varMap;
        varMap.insert(DEST, dest);
        varMap.insert(HOP_LIMIT, hopLimit);
        varMap.insert(CHAT_TEXT, chatText);

        sendMap(varMap, addr);
    }
    else
    {
        // There's no route table entry for dest
        qDebug() << "Cannot send private message to " << dest;
    }
}

void NetSocket::sendPrivate(QString& dest, QString& chatText)
{
    sendPrivate(dest, 10, chatText);
}

void NetSocket::sendRandRouteRumor()
{
    MessageInfo mesInf;
    mesInf.m_isRoute = true;
    mesInf.m_host = m_hostName;
    mesInf.m_seqNo = m_seqNo;

    m_seqNo++;

    AddrInfo addr(QHostAddress(QHostAddress::LocalHost), m_myPort);

    GlobalMessages->recordMessage(mesInf, addr);
    sendToRandNeighbor(mesInf);
}
