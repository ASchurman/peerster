#ifndef NETSOCKET_HH
#define NETSOCKET_HH

#include <QUdpSocket>
#include <QVariantMap>
#include <QMap>
#include <QList>
#include <QTimer>
#include <QByteArray>

#include "messageinfo.hh"
#include "addrinfo.hh"
#include "Monger.hh"
#include "PrivateMessage.hh"

// Handles the network communication of peerster
class NetSocket : public QUdpSocket
{
    Q_OBJECT

public:
    NetSocket();

    // Bind this socket to a Peerster-specific default port.
    bool bind();

    void inputMessage(QString& message);

    void sendToRandNeighbor(MessageInfo& mesInf);
    void sendMessage(MessageInfo& mesInf,
                     QHostAddress addresss,
                    int port,
                    bool startTimer = true);
    void sendStatus(QHostAddress address, int port);

    // Send a chat private message originating from this node
    void sendPrivate(QString& dest, QString& chatText);

    void sendSearchRequest(QString& searchTerms,
                           int budget,
                           QString origin = QString());
    void sendSearchReply(QString& searchTerms,
                         QList<QString>& fileNames,
                         QList<QByteArray>& hashes,
                         QString& dest);

    void addNeighbor(AddrInfo addrInfo);
    void addNeighbor(QString& hostPortStr);

    void noForward();
    bool m_forward;

    void requestBlock(QByteArray& hash, QString& host);

    void beginTrustChallenge(const QString& host,
                             const QString& question,
                             const QString& answer);

    QString m_hostName;

public slots:
    void gotReadyRead();
    void sendStatusToRandNeighbor();
    void lookedUpDns(const QHostInfo& host);

    // sends a route rumor message to a random neighbor
    void sendRandRouteRumor();

signals:
    void messageReceived(MessageInfo& mesInf);
    void gotSearchResult(QString& terms, QString& fileName, QByteArray& hash, QString& host);

private:
    // Send a private message of any type
    void sendPrivate(PrivateMessage* priv);
    void sendPrivate(const QVariantMap& priv);

    void sendMap(const QVariantMap& varMap, QHostAddress address, int port);
    void sendMap(const QVariantMap& varMap, const AddrInfo& addr);

    // finds the provided AddrInfo in m_neighborAddrs and returns a pointer to
    // it, else returns NULL
    Monger* findNeighbor(AddrInfo addrInfo);

    QList<AddrInfo> m_neighborAddrs;
    QList<Monger*> m_neighbors;
    QList<AddrInfo> m_pendingAddrs;

    int m_myPortMin, m_myPortMax, m_myPort;
    int m_seqNo;

    // timer for sending a status message to a random neighbor
    QTimer* m_statusTimer;

    // timer for sending a route rumor message to a random neighbor
    QTimer* m_routeTimer;
};

extern NetSocket* GlobalSocket;

#endif // NETSOCKET_HH
