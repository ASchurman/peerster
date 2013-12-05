#ifndef MESSAGESTORE_HH
#define MESSAGESTORE_HH

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "messageinfo.hh"
#include "addrinfo.hh"

class MessageStore : public QObject
{
    Q_OBJECT

public:
    MessageStore() {}

    // Returns the status of this instance. Status is encoded as a
    // QVariantMap keyed by host name. The value is the lowest message number
    // NOT YET seen by this instance from that host name.
    // EXAMPLE: <"hostA", 3>,<"hostB", 6>,<"hostC", 2>
    //          where 3 is the lowest message num not seen from hostA, etc.
    // The caller is responsible for deleting the returned QVariantMap.
    QVariantMap* getStatus();

    // Determines the difference between this instance's status and another
    //     status.
    // Returns 0 if they're the same, 1 if local host has extra messages,
    //     and -1 if remote host has extra messages.
    // Populates messageOut, hostNameOut, and messageNumOut with one of the
    //     extra messages on the local host if such a message exists
    int getStatusDiff(QVariantMap& remoteStatus, MessageInfo& mesInfOut);

    // Records a new message in internal data structures.
    // Returns true if the message is new, false if not.
    bool recordMessage(MessageInfo& mesInf, AddrInfo& addr, bool isDirect);

signals:
    // Indicates that a new message has been seen for the first time. mesInf
    // is that message, which arrived from addr.
    void newMessage(MessageInfo& mesInf, AddrInfo& addr, bool isDirect);

private:
    // Map keyed by host names.
    // The value QMaps are keyed by message number.
    QMap<QString, QMap<int, MessageInfo> > m_messages;
};

extern MessageStore* GlobalMessages;

#endif // MESSAGESTORE_HH
