#ifndef NETSOCKET_HH
#define NETSOCKET_HH

#include <QUdpSocket>
#include <QVariantMap>
#include <QMap>
#include <QList>

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
    void sendStatusToRandNeighbor();
    void sendMessage(MessageInfo& mesInf, QHostAddress addresss, int port);
    void sendStatus(QHostAddress address, int port);

public slots:
    void gotReadyRead();

signals:
    void messageReceived(MessageInfo& mesInf);

private:
    void sendMap(QVariantMap& varMap, QHostAddress address, int port);

    // finds the provided AddrInfo in m_sessionAddrs and returns a pointer to
    // it, else returns NULL
    Monger* findSession(AddrInfo addrInfo);

    QList<AddrInfo> m_neighbors;

    QList<AddrInfo> m_sessionAddrs;
    QList<Monger*> m_sessionMongers;

    int m_myPortMin, m_myPortMax, m_myPort;
    QString m_hostName;
    int m_seqNo;
};

extern NetSocket* GlobalSocket;

#endif // NETSOCKET_HH
