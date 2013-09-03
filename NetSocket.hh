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

    // Receives a chat message. Should only be called when the readyRead signal
    // is fired. The returned QString* should be deleted by the caller.
    // Returns NULL in error.
    QString* receive();

private:
    int m_myPortMin, m_myPortMax;
};

extern NetSocket GlobalSocket;

#endif // NETSOCKET_HH
