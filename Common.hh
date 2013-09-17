#ifndef COMMON_HH
#define COMMON_HH

#include <QString>
#include <QHostAddress>
#include <QHostInfo>

class MessageInfo
{
public:
    MessageInfo()
        : m_isRoute(false) { }

    MessageInfo(QString body, QString host, int seqNo)
        : m_isRoute(false), m_body(body), m_host(host), m_seqNo(seqNo) { }

    MessageInfo(QString host, int seqNo)
        : m_isRoute(true), m_host(host), m_seqNo(seqNo) { }

    void addBody(QString body)
    {
        m_body = body;
        m_isRoute = false;
    }

    // if true, this is a route rumor message and contains no body 
    bool m_isRoute;

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

    // True if this AddrInfo has host name in m_dns instead of an IP address in
    // m_addr. If false, m_addr is defined and m_dns may or may not be defined.
    bool m_isDns;

    QString m_dns;
    QHostAddress m_addr;
    int m_port;
};

#endif // COMMON_HH
