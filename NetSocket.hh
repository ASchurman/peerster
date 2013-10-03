#ifndef NETSOCKET_HH
#define NETSOCKET_HH

#include <QUdpSocket>
#include <QVariantMap>
#include <QMap>
#include <QList>
#include <QTimer>
#include <QByteArray>

#include "Common.hh"
#include "Monger.hh"

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

    // Send a private message originating from this node
    void sendPrivate(QString& dest, QString& chatText);

    void addNeighbor(AddrInfo addrInfo);
    void addNeighbor(QString& hostPortStr);

    void noForward();
    bool m_forward;

    void requestBlock(QByteArray& hash, QString& host);

    QString m_hostName;

public slots:
    void gotReadyRead();
    void sendStatusToRandNeighbor();
    void lookedUpDns(const QHostInfo& host);

    // sends a route rumor message to a random neighbor
    void sendRandRouteRumor();

signals:
    void messageReceived(MessageInfo& mesInf);

private:
    void sendPrivate(PrivateMessage& priv);

    void sendMap(QVariantMap& varMap, QHostAddress address, int port);
    void sendMap(QVariantMap& varMap, AddrInfo& addr);

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
