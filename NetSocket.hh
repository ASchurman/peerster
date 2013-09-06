#ifndef NETSOCKET_HH
#define NETSOCKET_HH

#include <QUdpSocket>
#include <QVariantMap>
#include <QMap>
#include <QList>
#include <QTimer>

#include "Common.hh"
#include "Monger.hh"

class NetSocket : public QUdpSocket
{
    Q_OBJECT

public:
    NetSocket();

    // Bind this socket to a Peerster-specific default port.
    bool bind();

    void inputMessage(QString& message);

    void sendToRandNeighbor(MessageInfo& mesInf);
    void sendMessage(MessageInfo& mesInf, QHostAddress addresss, int port);
    void sendStatus(QHostAddress address, int port);

    void addNeighbor(AddrInfo addrInfo);
    void addNeighbor(QString& hostPortStr);

public slots:
    void gotReadyRead();
    void sendStatusToRandNeighbor();
    void lookedUpDns(const QHostInfo& host);

signals:
    void messageReceived(MessageInfo& mesInf);

private:
    void sendMap(QVariantMap& varMap, QHostAddress address, int port);

    // finds the provided AddrInfo in m_neighborAddrs and returns a pointer to
    // it, else returns NULL
    Monger* findNeighbor(AddrInfo addrInfo);

    QList<AddrInfo> m_neighborAddrs;
    QList<Monger*> m_neighbors;
    QList<AddrInfo> m_pendingAddrs;

    int m_myPortMin, m_myPortMax, m_myPort;
    QString m_hostName;
    int m_seqNo;

    QTimer* m_timer;
};

extern NetSocket* GlobalSocket;

#endif // NETSOCKET_HH
