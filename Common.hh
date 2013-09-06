#ifndef COMMON_HH
#define COMMON_HH

#include <QString>
#include <QHostAddress>
#include <QHostInfo>

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
    AddrInfo() : m_isDns(false) { }
    AddrInfo(QHostAddress addr, int port)
        : m_isDns(false), m_addr(addr), m_port(port) { }
    AddrInfo(QString& dns, int port)
        : m_isDns(true), m_dns(dns), m_port(port) { }

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

    bool m_isDns;
    QString m_dns;

    QHostAddress m_addr;
    int m_port;
};

#endif // COMMON_HH
