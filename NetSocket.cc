#include <unistd.h>

#include <QDebug>
#include <QVariantList>
#include <QSet>

#include "NetSocket.hh"
#include "MessageStore.hh"
#include "RouteTable.hh"
#include "ChatDialog.hh"
#include "FileStore.hh"

NetSocket* GlobalSocket;

// VariantMap keys defined by the protocol
#define CHAT_TEXT "ChatText"
#define ORIGIN "Origin"
#define SEQ_NO "SeqNo"
#define WANT "Want"
#define DEST "Dest"
#define HOP_LIMIT "HopLimit"
#define LAST_IP "LastIP"
#define LAST_PORT "LastPort"
#define BLOCK_REQ "BlockRequest"
#define BLOCK_REP "BlockReply"
#define DATA "Data"
#define SEARCH "Search"
#define BUDGET "Budget"
#define SEARCH_REP "SearchReply"
#define MATCH_NAMES "MatchNames"
#define MATCH_IDS "MatchIDs"

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

    m_forward = true;
}

void NetSocket::noForward()
{
    qDebug() << "NO FORWARD";
    m_forward = false;
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
        if (varMap.contains(SEARCH)
                && varMap.contains(ORIGIN)
                && varMap.contains(BUDGET))
        {
            // received a search request!
            QString searchTerms = varMap[SEARCH].toString();
            QString origin = varMap[ORIGIN].toString();
            int budget = varMap[BUDGET].toInt();

            // Drop search requests that I sent
            if (origin == m_hostName)
            {
                return;
            }

            if (budget > 0)
            {
                qDebug() << "Received good search request: " << searchTerms;
                QList<QString> fileNames;
                QList<QByteArray> hashes;
                if (GlobalFiles->findFile(searchTerms, fileNames, hashes))
                {
                    qDebug() << "Found matches for search request; sending reply";
                    sendSearchReply(searchTerms, fileNames, hashes, origin);
                }

                budget--;
                if (budget > 0)
                {
                    sendSearchRequest(searchTerms, budget, origin);
                }
            }
            else
            {
                qDebug() << "Received search request w/budget <= 0";
            }
        }
        if (varMap.contains(ORIGIN)
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

            // LAST_PORT and LAST_IP to neighbors if they are provided in
            // the datagram
            bool isDirect = true;
            if (varMap.contains(LAST_PORT) && varMap.contains(LAST_IP))
            {
                isDirect = false;
                QHostAddress lastAddress(varMap[LAST_IP].toInt());
                int lastPort = varMap[LAST_PORT].toInt();
                AddrInfo lastAddrInfo(lastAddress, lastPort);

                if (!findNeighbor(lastAddrInfo))
                {
                    addNeighbor(lastAddrInfo);
                }
            }

            mesInf.addLastRoute(address.toIPv4Address(), (quint16)port);

            findNeighbor(addrInfo)->receiveMessage(mesInf, addrInfo, isDirect);
        }
        else if (varMap.contains(DEST)
                 && varMap.contains(HOP_LIMIT))
        {
            // received private message!

            // Get DEST, HOP_LIMIT, and ORIGIN
            QString dest(varMap[DEST].toString());
            int hopLimit = varMap[HOP_LIMIT].toInt();

            QString origin;
            if (varMap.contains(ORIGIN)) origin = varMap[ORIGIN].toString();

            // Construct PrivateMessage object by looking at the rest of the map
            PrivateMessage priv;
            if (varMap.contains(CHAT_TEXT))
            {
                QString chatText(varMap[CHAT_TEXT].toString());
                priv.chat(dest, hopLimit, chatText, origin);
            }
            else if (varMap.contains(BLOCK_REQ))
            {
                QByteArray blockReq = varMap[BLOCK_REQ].toByteArray();
                priv.blockRequest(dest, hopLimit, blockReq, origin);
            }
            else if (varMap.contains(BLOCK_REP))
            {
                QByteArray blockRep = varMap[BLOCK_REP].toByteArray();
                QByteArray data = varMap[DATA].toByteArray();
                priv.blockReply(dest, hopLimit, blockRep, data, origin);
            }
            else if (varMap.contains(SEARCH_REP)
                     && varMap.contains(MATCH_NAMES)
                     && varMap.contains(MATCH_IDS))
            {
                QString searchTerms = varMap[SEARCH_REP].toString();
                QVariantList resultNames = varMap[MATCH_NAMES].toList();
                QVariantList resultIds = varMap[MATCH_IDS].toList();
                priv.searchReply(dest, hopLimit, searchTerms, resultNames, resultIds, origin);
            }
            else
            {
                qDebug() << "Received private without content";
                return;
            }

            // Process the PrivateMessage we constructed
            if (dest == m_hostName)
            {
                // this private message is meant for me
                switch(priv.m_type)
                {
                    case PRIV_CHAT:
                    {
                        // don't print a private message that I send to myself;
                        // it already got printed when I sent it
                        if (priv.m_origin != m_hostName)
                        {
                            GlobalChatDialog->printPrivate(priv);
                        }
                        break;
                    }
                    case PRIV_BLOCKREQ:
                    {
                        qDebug() << "Got a block request";
                        QByteArray block;
                        if (GlobalFiles->findBlock(priv.m_hash, block))
                        {
                            PrivateMessage privReply(priv.m_origin,
                                                     10,
                                                     priv.m_hash,
                                                     block,
                                                     m_hostName);
                            sendPrivate(privReply);
                        }
                        break;
                    }
                    case PRIV_BLOCKREP:
                    {
                        GlobalFiles->addBlock(priv.m_hash, priv.m_data);
                        break;
                    }
                    case PRIV_SEARCHREP:
                    {
                        for (int i = 0; i < priv.m_resultFileNames.count(); i++)
                        {
                            QString fileName = priv.m_resultFileNames[i].toString();
                            QByteArray hash = priv.m_resultHashes[i].toByteArray();
                            emit gotSearchResult(priv.m_searchTerms,fileName, hash, priv.m_origin);
                        }
                        break;
                    }
                    default:
                    {
                        qDebug() << "Trying to process undef PrivateMessage";
                        break;
                    }
                }
            }
            else if (hopLimit - 1 > 0 && m_forward)
            {
                // Route the PrivateMessage to another host
                priv.m_hopLimit--;
                qDebug() << "Routing private w/DEST = " << dest;
                sendPrivate(priv);
            }
        }
        else if (varMap.contains(WANT))
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

    GlobalMessages->recordMessage(mesInf, addr, true/*isDirect*/);
    sendToRandNeighbor(mesInf);
}

void NetSocket::sendToRandNeighbor(MessageInfo& mesInf)
{
    // If this is a route rumor message, we're actually going to send it to
    // all our neighbors.
    if (mesInf.m_isRoute)
    {
        for (int i = 0; i < m_neighborAddrs.count(); i++)
        {
            AddrInfo addrInfo = m_neighborAddrs[i];

            // do not timeout on sending messages here, since we're sending
            // the route rumor to everyone anyway
            sendMessage(mesInf, addrInfo.m_addr, addrInfo.m_port, false);
        }
    }
    else
    {
        int i = rand() % m_neighborAddrs.count();
        AddrInfo addrInfo = m_neighborAddrs[i];

        sendMessage(mesInf, addrInfo.m_addr, addrInfo.m_port);
    }
}

void NetSocket::sendStatusToRandNeighbor()
{
    int i = rand() % m_neighborAddrs.count();
    AddrInfo addrInfo = m_neighborAddrs[i];

    sendStatus(addrInfo.m_addr, addrInfo.m_port);
}

void NetSocket::sendMessage(MessageInfo& mesInf,
                            QHostAddress address,
                            int port,
                            bool startTimer)
{
    QVariantMap varMap;
    if (!mesInf.m_isRoute) varMap.insert(CHAT_TEXT, mesInf.m_body);
    varMap.insert(ORIGIN, mesInf.m_host);
    varMap.insert(SEQ_NO, mesInf.m_seqNo);
    if (mesInf.m_hasLastRoute)
    {
        varMap.insert(LAST_IP, mesInf.m_lastIP);
        varMap.insert(LAST_PORT, mesInf.m_lastPort);
    }
    sendMap(varMap, address, port);

    AddrInfo addrInfo(address, port);
    if (!findNeighbor(addrInfo))
    {
        addNeighbor(addrInfo);
    }

    if (startTimer)
    {
        findNeighbor(addrInfo)->m_lastSent = mesInf;
        findNeighbor(addrInfo)->startTimer();
    }
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

void NetSocket::sendPrivate(PrivateMessage& priv)
{
    AddrInfo addr;

    if (GlobalRoutes->getNextHop(priv.m_dest, addr))
    {
        QVariantMap varMap;
        varMap.insert(DEST, priv.m_dest);
        varMap.insert(HOP_LIMIT, priv.m_hopLimit);
        if (priv.hasOrigin()) varMap.insert(ORIGIN, priv.m_origin);

        switch(priv.m_type)
        {
            case PRIV_CHAT:
                varMap.insert(CHAT_TEXT, priv.m_text);
                break;
            case PRIV_BLOCKREQ:
                varMap.insert(BLOCK_REQ, priv.m_hash);
                break;
            case PRIV_BLOCKREP:
                varMap.insert(BLOCK_REP, priv.m_hash);
                varMap.insert(DATA, priv.m_data);
                break;
            case PRIV_SEARCHREP:
                varMap.insert(SEARCH_REP, priv.m_searchTerms);
                varMap.insert(MATCH_NAMES, priv.m_resultFileNames);
                varMap.insert(MATCH_IDS, priv.m_resultHashes);
                break;
            default:
                qDebug() << "Trying to send undef PrivateMessage";
        }

        sendMap(varMap, addr);
    }
    else
    {
        // There's no route table entry for dest
        qDebug() << "Cannot send private message to " << priv.m_dest;
    }
}

// send a private message originating from this node
void NetSocket::sendPrivate(QString& dest, QString& chatText)
{
    PrivateMessage priv(dest, 10, chatText, m_hostName);
    GlobalChatDialog->printPrivate(priv);
    sendPrivate(priv);
}

void NetSocket::sendRandRouteRumor()
{
    MessageInfo mesInf;
    mesInf.m_isRoute = true;
    mesInf.m_host = m_hostName;
    mesInf.m_seqNo = m_seqNo;

    m_seqNo++;

    AddrInfo addr(QHostAddress(QHostAddress::LocalHost), m_myPort);

    GlobalMessages->recordMessage(mesInf, addr, true/*isDirect*/);
    sendToRandNeighbor(mesInf);
}

void NetSocket::requestBlock(QByteArray& hash, QString& host)
{
    PrivateMessage priv(host, 10, hash, m_hostName);
    sendPrivate(priv);
}

void NetSocket::sendSearchRequest(QString &searchTerms,
                                  int budget,
                                  QString origin)
{
    if (origin.isEmpty()) origin = m_hostName;

    int baseBudget = budget / m_neighborAddrs.count();
    int extraBudget = budget % m_neighborAddrs.count();

    QList<int> budgetAlloc;
    if (baseBudget > 0)
    {
        // We have enough budget for everyone to get baseBudget
        for (int i = 0; i < m_neighborAddrs.count(); i++) budgetAlloc.append(baseBudget);

        // Now spread extraBudget among as many peers as we can
        for (int i = 0; i < extraBudget; i++) budgetAlloc[i]++;
    }
    else
    {
        // We don't have enough budget for everyone to get some budget; just
        // spread extraBudget to as many peers as we can
        for (int i = 0; i < extraBudget; i++) budgetAlloc.append(1);
    }

    // Now send the messages with the calculated budgets
    QList<AddrInfo> neighbors = m_neighborAddrs;
    if (budgetAlloc.count() > neighbors.count())
    {
        qDebug() << "BUG!!! budgetAlloc.count() > neighbors.count() !!!!!!";
        return;
    }
    for (int i = 0; i < budgetAlloc.count(); i++)
    {
        QVariantMap varMap;
        varMap.insert(ORIGIN, origin);
        varMap.insert(SEARCH, searchTerms);
        varMap.insert(BUDGET, budgetAlloc[i]);

        int j = rand() % neighbors.count();
        qDebug() << "Sending budget " << budgetAlloc[i]
                    << " to neighbor " << neighbors[j].m_addr.toString();
        AddrInfo addrInfo = neighbors[j];
        sendMap(varMap, addrInfo);
        neighbors.removeAt(j);
    }
}

void NetSocket::sendSearchReply(QString &searchTerms,
                                QList<QString> &fileNames,
                                QList<QByteArray> &hashes,
                                QString &dest)
{
    QVariantList varFileNames;
    QVariantList varHashes;
    for (int i = 0; i < fileNames.count(); i++)
    {
        varFileNames.append(fileNames[i]);
        varHashes.append(hashes[i]);
    }
    PrivateMessage priv(dest, 10, searchTerms, varFileNames, varHashes, m_hostName);
    sendPrivate(priv);
}
