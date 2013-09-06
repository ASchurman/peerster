#ifndef MONGER_HH
#define MONGER_HH

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QHostAddress>
#include <QTimer>

#include "Common.hh"

class Monger : public QObject
{
    Q_OBJECT

public:
    Monger();
    Monger(AddrInfo addrInfo);

    void receiveMessage(MessageInfo mesInf);

    void receiveStatus(QVariantMap remoteStatus);

    void startTimer();

    AddrInfo m_addrInfo;

public slots:
    void timeout();

private:
    void setupTimer();
    QTimer* m_pTimer;
};

#endif // MONGER_HH
