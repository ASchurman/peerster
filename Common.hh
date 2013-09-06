#ifndef COMMON_HH
#define COMMON_HH

#include <QString>
#include <QHostAddress>

class MessageInfo
{
public:
    MessageInfo() { }
    MessageInfo(QString body, QString host, int seqNo)
        : m_body(body), m_host(host), m_seqNo(seqNo) { }

    QString m_body, m_host;
    int m_seqNo;
};

class AddrInfo
{
public:
    AddrInfo() { }
    AddrInfo(QHostAddress addr, int port)
        : m_addr(addr), m_port(port) { }

    bool operator==(AddrInfo addrInfo)
    {
        if (m_addr == addrInfo.m_addr && m_port == addrInfo.m_port)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    QHostAddress m_addr;
    int m_port;
};

#endif // COMMON_HH
