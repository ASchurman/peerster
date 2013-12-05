#include "messageinfo.hh"

MessageInfo::MessageInfo()
{
    m_isRoute = false;
    m_hasLastRoute = false;
    m_hasSig = false;
    m_goodSig = false;
}

MessageInfo::MessageInfo(const QString& body, const QString& host, int seqNo)
{
    m_isRoute = false;
    m_hasLastRoute = false;
    m_hasSig = false;
    m_goodSig = false;
    m_body = body;
    m_host = host;
    m_seqNo = seqNo;
}

MessageInfo::MessageInfo(const QString& host, int seqNo)
{
    m_isRoute = true;
    m_hasLastRoute = false;
    m_hasSig = false;
    m_goodSig = false;
    m_host = host;
    m_seqNo = seqNo;
}

void MessageInfo::addBody(const QString& body)
{
    m_body = body;
    m_isRoute = false;
}

void MessageInfo::addLastRoute(quint32 lastIP, quint16 lastPort)
{
    m_hasLastRoute = true;
    m_lastIP = lastIP;
    m_lastPort = lastPort;
}

void MessageInfo::addSig(const QByteArray& sig, bool goodSig)
{
    m_hasSig = true;
    m_goodSig = goodSig;
    m_sig = sig;
}
