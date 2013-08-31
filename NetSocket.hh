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

private:
    int myPortMin, myPortMax;
};

#endif // NETSOCKET_HH
