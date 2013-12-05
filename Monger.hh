#ifndef MONGER_HH
#define MONGER_HH

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QHostAddress>
#include <QTimer>

#include "messageinfo.hh"
#include "addrinfo.hh"

// Contains the per-neighbor state
class Monger : public QObject
{
    Q_OBJECT

public:
    Monger();
    Monger(AddrInfo addrInfo);

    void receiveMessage(MessageInfo mesInf, AddrInfo& addrInfo, bool isDirect);

    void receiveStatus(QVariantMap remoteStatus);

    void startTimer();

    AddrInfo m_addrInfo;

    // last message sent to this peer; resent to a random neighbor if this
    // neighbor times out
    MessageInfo m_lastSent;

public slots:
    void timeout();

private:
    void setupTimer();
    QTimer* m_pTimer;
};

#endif // MONGER_HH
