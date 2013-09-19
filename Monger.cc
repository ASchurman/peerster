#include <QDebug>

#include "Monger.hh"
#include "NetSocket.hh"
#include "MessageStore.hh"

Monger::Monger()
{
    setupTimer();
}

Monger::Monger(AddrInfo addrInfo)
{
    m_addrInfo = addrInfo;
    setupTimer();
}

void Monger::setupTimer()
{
    m_pTimer = new QTimer(this);
    m_pTimer->setSingleShot(true);
    m_pTimer->setInterval(2000);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(timeout()));
}

void Monger::startTimer()
{
    m_pTimer->start();
}

void Monger::timeout()
{
    qDebug() << "Timeout";
    int resend = rand() % 2;
    if (resend
        && (GlobalSocket->m_forward || m_lastSent.m_isRoute))
    {
        GlobalSocket->sendToRandNeighbor(m_lastSent);
    }
}

void Monger::receiveMessage(MessageInfo mesInf, AddrInfo& addrInfo)
{
    if (GlobalMessages->recordMessage(mesInf, addrInfo))
    {
        // this is a new rumor
        if (GlobalSocket->m_forward || mesInf.m_isRoute)
        {
            GlobalSocket->sendToRandNeighbor(mesInf);
        }
    }

    // now reply with status
    GlobalSocket->sendStatus(m_addrInfo.m_addr, m_addrInfo.m_port);
}

void Monger::receiveStatus(QVariantMap remoteStatus)
{
    MessageInfo mesInf;
    m_pTimer->stop();

    int statusDiff = GlobalMessages->getStatusDiff(remoteStatus, mesInf);

    if (statusDiff < 0)
    {
        // remote host has messages we don't, so send status message
        GlobalSocket->sendStatus(m_addrInfo.m_addr, m_addrInfo.m_port);
    }
    else if (statusDiff > 0)
    {
        // we have messages the remote host doesn't, so send one
        if (GlobalSocket->m_forward || mesInf.m_isRoute)
        {
            GlobalSocket->sendMessage(mesInf, m_addrInfo.m_addr, m_addrInfo.m_port);
        }
    }
    else // statusDiff == 0
    {
        // we have the same set of messages
        int resend = rand() % 2;
        if (resend)
        {
            GlobalSocket->sendStatusToRandNeighbor();
        }
    }
}
