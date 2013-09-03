#ifndef NETSOCKET_HH
#define NETSOCKET_HH

#include <QUdpSocket>

class NetSocket : public QUdpSocket
{
    Q_OBJECT

public:
    NetSocket();

    // Bind this socket to a Peerster-specific default port.
    bool bind();

    void send(QString& message);

public slots:
    void gotReadyRead();

signals:
    void messageReceived(QString& message);

private:
    int m_myPortMin, m_myPortMax;
};

extern NetSocket* GlobalSocket;

#endif // NETSOCKET_HH
