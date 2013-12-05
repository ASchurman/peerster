#ifndef ADDRINFO_HH
#define ADDRINFO_HH

#include <QString>
#include <QHostAddress>
#include <QHostInfo>

class AddrInfo
{
public:
    AddrInfo() : m_isDns(false) { }
    AddrInfo(QHostAddress addr, int port)
        : m_isDns(false), m_addr(addr), m_port(port) { }
    AddrInfo(QString& dns, int port)
        : m_isDns(true), m_dns(dns), m_port(port) { }

    bool operator==(AddrInfo addrInfo);

    // True if this AddrInfo has host name in m_dns instead of an IP address in
    // m_addr. If false, m_addr is defined and m_dns may or may not be defined.
    bool m_isDns;

    QString m_dns;
    QHostAddress m_addr;
    int m_port;
};

#endif // ADDRINFO_HH
