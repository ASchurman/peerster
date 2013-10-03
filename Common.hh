#ifndef COMMON_HH
#define COMMON_HH

#include <QString>
#include <QHostAddress>
#include <QHostInfo>
#include <QByteArray>

class MessageInfo
{
public:
    MessageInfo()
    {
        m_isRoute = false;
        m_hasLastRoute = false;
    }

    MessageInfo(QString body, QString host, int seqNo)
    {
        m_isRoute = false;
        m_hasLastRoute = false;
        m_body = body;
        m_host = host;
        m_seqNo = seqNo;
    }

    MessageInfo(QString host, int seqNo)
    {
        m_isRoute = true;
        m_hasLastRoute = false;
        m_host = host;
        m_seqNo = seqNo;
    }

    void addBody(QString body)
    {
        m_body = body;
        m_isRoute = false;
    }

    void addLastRoute(quint32 lastIP, quint16 lastPort)
    {
        m_hasLastRoute = true;
        m_lastIP = lastIP;
        m_lastPort = lastPort;
    }

    // if true, this is a route rumor message and contains no body 
    bool m_isRoute;

    QString m_body, m_host;
    int m_seqNo;

    bool m_hasLastRoute;
    quint32 m_lastIP;
    quint16 m_lastPort;
};

#define PRIV_UNDEF (0)
#define PRIV_CHAT (1)
#define PRIV_BLOCKREQ (2)
#define PRIV_BLOCKREP (3)

class PrivateMessage
{
public:
    PrivateMessage() { m_type = PRIV_UNDEF; }

    PrivateMessage(QString& dest,
                   int hopLimit,
                   QString& text,
                   QString origin = QString())
    {
        chat(dest, hopLimit, text, origin);
    }
    void chat(QString& dest,
              int hopLimit,
              QString& text,
              QString origin = QString())
    {
        m_type = PRIV_CHAT;
        m_dest = dest;
        m_hopLimit = hopLimit;
        m_text = text;
        m_origin = origin;
    }

    PrivateMessage(QString& dest,
                   int hopLimit,
                   QByteArray& blockReq,
                   QString origin = QString())
    {
        blockRequest(dest, hopLimit, blockReq, origin);
    }
    void blockRequest(QString& dest,
                      int hopLimit,
                      QByteArray& blockReq,
                      QString origin = QString())
    {
        m_type = PRIV_BLOCKREQ;
        m_dest = dest;
        m_hopLimit = hopLimit;
        m_hash = blockReq;
        m_origin = origin;
    }

    PrivateMessage(QString& dest,
                   int hopLimit,
                   QByteArray& blockRep,
                   QByteArray& data,
                   QString origin = QString())
    {
        blockReply(dest, hopLimit, blockRep, data, origin);
    }
    void blockReply(QString& dest,
                    int hopLimit,
                    QByteArray& blockRep,
                    QByteArray& data,
                    QString origin = QString())
    {
        m_type = PRIV_BLOCKREP;
        m_dest = dest;
        m_hopLimit = hopLimit;
        m_hash = blockRep;
        m_data = data;
        m_origin = origin;
    }

    // equal to one of the PRIV macros above
    int m_type;

    // Defined for all private messages
    QString m_dest;
    int m_hopLimit;

    // Defined for chat privates
    QString m_text;

    // Defined for BLOCKREQ and BLOCKREP
    QByteArray m_hash;

    // Defined for PRIV_BLOCKREP
    QByteArray m_data;

    // ORIGIN value which may not be defined for incoming messages
    bool hasOrigin() { return !m_origin.isEmpty(); }
    QString m_origin;
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
