#include <unistd.h>

#include <QDebug>
#include <QVariantList>
#include <QSet>

#include "NetSocket.hh"
#include "MessageStore.hh"
#include "RouteTable.hh"
#include "ChatDialog.hh"
#include "FileStore.hh"
#include "finalProject/crypto.hh"

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

// Crypto-related VariantMap keys
#define MESSAGE "Message"
#define SIG "Sig"
#define PUBKEY "PubKey"
#define PUBKEY_SIGNERS "PubKeySigners"
#define CRYPT_DATA "CryptData"
#define CRYPT_KEY "CryptKey"
#define CHALLENGE "Challenge"
#define CRYPT_PUBKEY "CryptPubKey"
#define PUBKEY_SIG "PubKeySig"
#define SIG_REQ "SigRequest"
#define SIGNER "Signer"
#define SIG_REP "SigResponse"

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
    m_hostName.prepend(QHostInfo::localHostName());

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

        if (varMap.contains(HOP_LIMIT))
        {
            // This is a point-to-point message

            // Extract the top-level entries
            int hopLimit = varMap[HOP_LIMIT].toInt();
            QVariantMap mes = varMap[MESSAGE].toMap();
            QByteArray sig = varMap[SIG].toByteArray();

            // Extract DEST, ORIGIN, and CRYPT from the MESSAGE
            QString dest(mes[DEST].toString());
            QString origin(mes[ORIGIN].toString());
            QByteArray crypt = mes[CRYPT_DATA].toByteArray();
            QByteArray cryptKey = mes[CRYPT_KEY].toByteArray();

            if (dest == m_hostName)
            {
                // I am the recipient of this private message!

                // Decrypt the message
                QVariantMap decrypted = GlobalCrypto->decryptMap(crypt, cryptKey);
                if (decrypted.isEmpty())
                {
                    qDebug() << "Decrypted map is empty!!";
                    return;
                }

                // Construct a PrivateMessage object by looking at the decrypted
                // map
                PrivateMessage* priv = NULL;
                if (decrypted.contains(CHAT_TEXT))
                {
                    QString chatText(decrypted[CHAT_TEXT].toString());
                    priv = new PrivateChat(dest, hopLimit, chatText, origin);
                }
                else if (decrypted.contains(BLOCK_REQ))
                {
                    QByteArray blockReq = decrypted[BLOCK_REQ].toByteArray();
                    priv = new PrivateBlockReq(dest, hopLimit, blockReq, origin);
                }
                else if (decrypted.contains(BLOCK_REP))
                {
                    QByteArray blockRep = decrypted[BLOCK_REP].toByteArray();
                    QByteArray data = decrypted[DATA].toByteArray();
                    priv = new PrivateBlockRep(dest, hopLimit, blockRep, data, origin);
                }
                else if (decrypted.contains(SEARCH_REP)
                         && decrypted.contains(MATCH_NAMES)
                         && decrypted.contains(MATCH_IDS))
                {
                    QString searchTerms = decrypted[SEARCH_REP].toString();
                    QVariantList resultNames = decrypted[MATCH_NAMES].toList();
                    QVariantList resultIds = decrypted[MATCH_IDS].toList();
                    priv = new PrivateSearchRep(dest,
                                                hopLimit,
                                                searchTerms,
                                                resultNames,
                                                resultIds,
                                                origin);
                }
                else if (decrypted.contains(CHALLENGE))
                {
                    QString challenge = decrypted[CHALLENGE].toString();
                    priv = new PrivateChallenge(dest, hopLimit, origin, challenge);
                }
                else if (decrypted.contains(CRYPT_PUBKEY))
                {
                    QByteArray cryptPubKey = decrypted[CRYPT_PUBKEY].toByteArray();
                    priv = new PrivateChallengeRep(dest, hopLimit, origin, cryptPubKey);
                }
                else if (decrypted.contains(PUBKEY_SIG))
                {
                    QByteArray sig = decrypted[PUBKEY_SIG].toByteArray();
                    priv = new PrivateChallengeSig(dest, hopLimit, origin, sig);
                }
                else if (decrypted.contains(SIG_REQ))
                {
                    QString name = decrypted[SIG_REQ].toString();
                    priv = new PrivateSigReq(dest, hopLimit, origin, name);
                }
                else if (decrypted.contains(SIG_REP))
                {
                    QString name = decrypted[SIGNER].toString();
                    QByteArray sig = decrypted[SIG_REP].toByteArray();
                    priv = new PrivateSigRep(dest, hopLimit, origin, name, sig);
                }
                else
                {
                    qDebug() << "Received private without content";
                    return;
                }

                // Verify the signature
                priv->m_validSig = GlobalCrypto->checkSig(origin, mes, sig);
                if (priv->m_validSig)
                {
                    qDebug() << "VALID SIG IN PRIVATE FROM " << origin;
                }
                else
                {
                    qDebug() << "INVALID SIG IN PRIVATE FROM " << origin;
                }

                // Process the PrivateMessage we constructed. Ignore privates
                // with invalid signatures, except chat messages, which we'll
                // display differently to the user in GlobalChatDialog
                if (priv->m_validSig || priv->type() == PrivateMessage::Chat)
                {
                    switch(priv->type())
                    {
                        case PrivateMessage::Chat:
                        {
                            PrivateChat* privChat = (PrivateChat*)priv;

                            // don't print a private message that I send to myself;
                            // it already got printed when I sent it
                            if (priv->m_origin != m_hostName)
                            {
                                GlobalChatDialog->printPrivate(*privChat);
                            }
                            break;
                        }
                        case PrivateMessage::BlockReq:
                        {
                            PrivateBlockReq* blockReq = (PrivateBlockReq*)priv;

                            qDebug() << "Got a block request";
                            QByteArray block;
                            if (GlobalFiles->findBlock(blockReq->m_hash, block))
                            {
                                PrivateBlockRep blockRep(blockReq->m_origin,
                                                         10,
                                                         blockReq->m_hash,
                                                         block,
                                                         m_hostName);
                                sendPrivate(&blockRep);
                            }
                            break;
                        }
                        case PrivateMessage::BlockRep:
                        {
                            PrivateBlockRep* blockRep = (PrivateBlockRep*)priv;
                            GlobalFiles->addBlock(blockRep->m_hash, blockRep->m_data);
                            break;
                        }
                        case PrivateMessage::SearchRep:
                        {
                            PrivateSearchRep* searchRep = (PrivateSearchRep*)priv;

                            for (int i = 0; i < searchRep->m_resultFileNames.count(); i++)
                            {
                                QString fileName = searchRep->m_resultFileNames[i].toString();
                                QByteArray hash = searchRep->m_resultHashes[i].toByteArray();
                                emit gotSearchResult(searchRep->m_searchTerms,
                                                     fileName,
                                                     hash,
                                                     searchRep->m_origin);
                            }
                            break;
                        }
                        case PrivateMessage::Challenge:
                        {
                            // Get an answer to the challenge question. If it's
                            // nonempty, use it to encrypt my public key and
                            // reply with it.
                            PrivateChallenge* chal = (PrivateChallenge*)priv;
                            QString answer = GlobalChatDialog->getChallengeAnswer(chal->m_origin,
                                                                                  chal->m_challenge);
                            if (!answer.isEmpty())
                            {
                                QByteArray cryptPubKey = GlobalCrypto->encryptKey(answer);
                                PrivateChallengeRep rep(chal->m_origin,
                                                        10,
                                                        m_hostName,
                                                        cryptPubKey);
                                sendPrivate(&rep);
                            }
                            break;
                        }
                        case PrivateMessage::ChallengeResponse:
                        {
                            // Verify the response. If valid, sign his public
                            // key and reply with the signature
                            PrivateChallengeRep* rep = (PrivateChallengeRep*)priv;
                            bool pass = GlobalCrypto->endChallenge(rep->m_origin,
                                                                   rep->m_response);
                            if (pass)
                            {
                                // He passed, so let's sign his key!
                                QByteArray sig = GlobalCrypto->signKey(rep->m_origin);
                                PrivateChallengeSig chalSig(rep->m_origin,
                                                            10,
                                                            m_hostName,
                                                            sig);
                                sendPrivate(&chalSig);
                            }
                            break;
                        }
                        case PrivateMessage::ChallengeSig:
                        {
                            // Add the signature of my public key to the store
                            PrivateChallengeSig* challengeSig = (PrivateChallengeSig*)priv;
                            GlobalCrypto->addKeySig(challengeSig->m_origin,
                                                    challengeSig->m_sig);
                            break;
                        }
                        case PrivateMessage::SignatureRequest:
                        {
                            // If the requested user signed our key, reply with
                            // the signature
                            PrivateSigReq* sigReq = (PrivateSigReq*)priv;
                            QByteArray sig = GlobalCrypto->getKeySig(sigReq->m_name);
                            if (!sig.isEmpty())
                            {
                                PrivateSigRep sigRep(sigReq->m_origin,
                                                     10,
                                                     m_hostName,
                                                     sigReq->m_name,
                                                     sig);
                                sendPrivate(&sigRep);
                            }
                            break;
                        }
                        case PrivateMessage::SignatureResponse:
                        {
                            // Verify the signature. If valid, and we trust the
                            // signer, then trust the sender and sign his key.
                            PrivateSigRep* sigRep = (PrivateSigRep*)priv;
                            if (GlobalCrypto->addTrust(sigRep->m_origin,
                                                       sigRep->m_name,
                                                       sigRep->m_sig))
                            {
                                // Valid signature by a trusted individual! Now
                                // sign the sender's key
                                QByteArray sig = GlobalCrypto->signKey(sigRep->m_origin);
                                PrivateChallengeSig chalSig(sigRep->m_origin,
                                                            10,
                                                            m_hostName,
                                                            sig);
                                sendPrivate(&chalSig);
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
                if (priv) delete priv;
            }
            else if (hopLimit - 1 > 0 && m_forward)
            {
                // Forward this private message
                qDebug() << "Routing private w/DEST = " << dest << ", HOP_LIMIT = " << hopLimit-1;
                varMap[HOP_LIMIT] = hopLimit - 1;
                sendPrivate(varMap);
            }
        }
        else if (varMap.contains(SEARCH))
        {
            // This is a search message
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
        else if (varMap.contains(WANT))
        {
            // This is a status message
            QVariantMap remoteStatus(varMap[WANT].toMap());

            if (!findNeighbor(addrInfo))
            {
                addNeighbor(addrInfo);
            }
            findNeighbor(addrInfo)->receiveStatus(remoteStatus);
        }
        else if (varMap.contains(MESSAGE))
        {
            // This is a rumor message

            // Add sender to neighbors
            if (!findNeighbor(addrInfo))
            {
                addNeighbor(addrInfo);
            }

            // Extract top-level entries in map
            QVariantMap mesMap = varMap[MESSAGE].toMap();
            QByteArray sig = varMap[SIG].toByteArray();
            QByteArray pubKey = varMap[PUBKEY].toByteArray();
            QList<QVariant> signers = varMap[PUBKEY_SIGNERS].toList();

            // Create and populate MessageInfo with chat text, signature,
            // lastRoute
            MessageInfo mesInf(mesMap[ORIGIN].toString(), mesMap[SEQ_NO].toInt());

            if (mesMap.contains(CHAT_TEXT))
            {
                mesInf.addBody(mesMap[CHAT_TEXT].toString());
            }

            // Before checking signature, add the public key to our store of
            // public keys
            GlobalCrypto->addPubKey(mesInf.m_host, pubKey);

            bool validSig = GlobalCrypto->checkSig(mesInf.m_host, mesMap, sig);
            mesInf.addSig(sig, validSig);
            if (validSig) qDebug() << "VALID SIGNATURE FROM " << mesInf.m_host;

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

            // Update our key sig list for the origin of this message
            GlobalCrypto->updateKeySigList(mesInf.m_host, signers);

            // If we don't already trust this individual, check their key
            // signers to see if we trust any of them. If so, request the sig
            if (validSig && !GlobalCrypto->isTrusted(mesInf.m_host))
            {
                // check signers one by one to see if we trust any
                for (int i = 0; i < signers.count(); i++)
                {
                    if (GlobalCrypto->isTrusted(signers[i].toString()))
                    {
                        // Request the signature of this individual we trust
                        PrivateSigReq sigReq(mesInf.m_host,
                                             10,
                                             m_hostName,
                                             signers[i].toString());
                        sendPrivate(&sigReq);
                        break;
                    }
                }
            }

            // Register this message
            findNeighbor(addrInfo)->receiveMessage(mesInf, addrInfo, isDirect);
        }
        else
        {
            // Not a point-to-point, search request, status, or rumor message.
            // Something went wrong!
            qDebug() << "RECEIVED INVALID MESSAGE TYPE";
        }
    }
}

void NetSocket::inputMessage(QString& message)
{
    MessageInfo mesInf(message, m_hostName, m_seqNo);

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
    QVariantMap mes;
    if (!mesInf.m_isRoute) mes.insert(CHAT_TEXT, mesInf.m_body);
    mes.insert(ORIGIN, mesInf.m_host);
    mes.insert(SEQ_NO, mesInf.m_seqNo);
    if (mesInf.m_hasLastRoute)
    {
        mes.insert(LAST_IP, mesInf.m_lastIP);
        mes.insert(LAST_PORT, mesInf.m_lastPort);
    }

    QVariantMap varMap;
    varMap.insert(MESSAGE, mes);
    if (mesInf.m_host == m_hostName)
    {
        varMap.insert(SIG, GlobalCrypto->sign(mes));
        varMap.insert(PUBKEY, GlobalCrypto->pubKeyVal());
        varMap.insert(PUBKEY_SIGNERS, GlobalCrypto->keySigList());
    }
    else
    {
        varMap.insert(SIG, mesInf.m_sig);
        varMap.insert(PUBKEY, GlobalCrypto->pubKeyVal(mesInf.m_host));
        varMap.insert(PUBKEY_SIGNERS, GlobalCrypto->keySigList(mesInf.m_sig));
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
    statusMessage.insert(ORIGIN, m_hostName);
    delete status;

    // TODO change status message to new standard
    /*QVariantMap out;
    out.insert(PUBKEY, GlobalCrypto->pubKeyVal());
    out.insert(PUBKEY_SIGNERS, GlobalCrypto->keySigList());
    out.insert(SIG, GlobalCrypto->sign(statusMessage));
    out.insert(MESSAGE, statusMessage);
    sendMap(out, address, port);*/
    sendMap(statusMessage, address, port);
}

void NetSocket::sendMap(const QVariantMap& varMap, QHostAddress address, int port)
{
    QByteArray datagram;
    datagram.resize(sizeof(varMap));
    QDataStream dataStream(&datagram, QIODevice::WriteOnly);
    dataStream << varMap;

    writeDatagram(datagram, address, port);
}

void NetSocket::sendMap(const QVariantMap& varMap, const AddrInfo& addr)
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

void NetSocket::sendPrivate(PrivateMessage* priv)
{
    AddrInfo addr;

    if (GlobalRoutes->getNextHop(priv->m_dest, addr))
    {
        // Make top-level map, out
        QVariantMap out;
        out.insert(HOP_LIMIT, priv->m_hopLimit);

        // Make MESSAGE map, contained in the top-level map
        QVariantMap mes;
        mes.insert(DEST, priv->m_dest);
        if (priv->hasOrigin()) mes.insert(ORIGIN, priv->m_origin);

        // Make the CRYPT map and encrypt it after populating it
        QVariantMap crypt;
        switch(priv->type())
        {
            case PrivateMessage::Chat:
            {
                PrivateChat* privChat = (PrivateChat*)priv;
                crypt.insert(CHAT_TEXT, privChat->m_text);
                break;
            }
            case PrivateMessage::BlockReq:
            {
                PrivateBlockReq* blockReq = (PrivateBlockReq*)priv;
                crypt.insert(BLOCK_REQ, blockReq->m_hash);
                break;
            }
            case PrivateMessage::BlockRep:
            {
                PrivateBlockRep* blockRep = (PrivateBlockRep*)priv;
                crypt.insert(BLOCK_REP, blockRep->m_hash);
                crypt.insert(DATA, blockRep->m_data);
                break;
            }
            case PrivateMessage::SearchRep:
            {
                PrivateSearchRep* searchRep = (PrivateSearchRep*)priv;
                crypt.insert(SEARCH_REP, searchRep->m_searchTerms);
                crypt.insert(MATCH_NAMES, searchRep->m_resultFileNames);
                crypt.insert(MATCH_IDS, searchRep->m_resultHashes);
                break;
            }
            case PrivateMessage::Challenge:
            {
                PrivateChallenge* challenge = (PrivateChallenge*)priv;
                crypt.insert(CHALLENGE, challenge->m_challenge);
                break;
            }
            case PrivateMessage::ChallengeResponse:
            {
                PrivateChallengeRep* challengeRep = (PrivateChallengeRep*)priv;
                crypt.insert(CRYPT_PUBKEY, challengeRep->m_response);
                break;
            }
            case PrivateMessage::ChallengeSig:
            {
                PrivateChallengeSig* challengeSig = (PrivateChallengeSig*)priv;
                crypt.insert(PUBKEY_SIG, challengeSig->m_sig);
                break;
            }
            case PrivateMessage::SignatureRequest:
            {
                PrivateSigReq* sigReq = (PrivateSigReq*)priv;
                crypt.insert(SIG_REQ, sigReq->m_name);
                break;
            }
            case PrivateMessage::SignatureResponse:
            {
                PrivateSigRep* sigRep = (PrivateSigRep*)priv;
                crypt.insert(SIGNER, sigRep->m_name);
                crypt.insert(SIG_REP, sigRep->m_sig);
                break;
            }
            default:
            {
                qDebug() << "Trying to send undef PrivateMessage";
                return;
            }
        }

        QByteArray cryptKey;
        QByteArray cryptArray = GlobalCrypto->encrypt(priv->m_dest,
                                                      crypt,
                                                      &cryptKey);

        mes.insert(CRYPT_DATA, cryptArray);
        mes.insert(CRYPT_KEY, cryptKey);
        out.insert(MESSAGE, mes);

        // Make signature and add it to the top-level map
        QByteArray sig = GlobalCrypto->sign(mes);
        out.insert(SIG, sig);

        sendMap(out, addr);
    }
    else
    {
        // There's no route table entry for dest
        qDebug() << "Cannot send private message to " << priv->m_dest;
    }
}

void NetSocket::sendPrivate(const QVariantMap& priv)
{
    AddrInfo addr;
    QString dest = priv[MESSAGE].toMap()[DEST].toString();

    if (GlobalRoutes->getNextHop(dest, addr))
    {
        sendMap(priv, addr);
    }
    else
    {
        // There's no route table entry for dest
        qDebug() << "Cannot send private message to " << dest;
    }
}

// send a private message originating from this node
void NetSocket::sendPrivate(QString& dest, QString& chatText)
{
    PrivateChat priv(dest, 10, chatText, m_hostName);
    GlobalChatDialog->printPrivate(priv);
    sendPrivate(&priv);
}

void NetSocket::sendRandRouteRumor()
{
    MessageInfo mesInf(m_hostName, m_seqNo);

    m_seqNo++;

    AddrInfo addr(QHostAddress(QHostAddress::LocalHost), m_myPort);

    GlobalMessages->recordMessage(mesInf, addr, true/*isDirect*/);
    sendToRandNeighbor(mesInf);
}

void NetSocket::requestBlock(QByteArray& hash, QString& host)
{
    PrivateBlockReq priv(host, 10, hash, m_hostName);
    sendPrivate(&priv);
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
        // TODO: Change search request map to conform to new standard (keep BUDGET outside of sig)
        QVariantMap varMap;
        varMap.insert(ORIGIN, origin);
        varMap.insert(SEARCH, searchTerms);
        varMap.insert(BUDGET, budgetAlloc[i]);

        int j = rand() % neighbors.count();
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
    PrivateSearchRep priv(dest, 10, searchTerms, varFileNames, varHashes, m_hostName);
    sendPrivate(&priv);
}

void NetSocket::beginTrustChallenge(const QString& host,
                                    const QString& question,
                                    const QString& answer)
{
    GlobalCrypto->startChallenge(host, answer);
    PrivateChallenge priv(host, 10, m_hostName, question);
    sendPrivate(&priv);
}
