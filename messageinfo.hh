#ifndef MESSAGEINFO_HH
#define MESSAGEINFO_HH

#include <QString>
#include <QByteArray>

class MessageInfo
{
public:
    MessageInfo();

    MessageInfo(const QString& body, const QString& host, int seqNo);

    MessageInfo(const QString& host, int seqNo);

    void addBody(const QString& body);
    void addLastRoute(quint32 lastIP, quint16 lastPort);
    void addSig(const QByteArray& sig, bool goodSig);

    // if true, this is a route rumor message and contains no body
    bool m_isRoute;

    QString m_body, m_host;
    int m_seqNo;

    bool m_hasLastRoute;
    quint32 m_lastIP;
    quint16 m_lastPort;

    bool m_hasSig;
    bool m_goodSig;
    QByteArray m_sig;
};

#endif // MESSAGEINFO_HH
