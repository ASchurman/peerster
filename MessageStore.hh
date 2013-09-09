#ifndef MESSAGESTORE_HH
#define MESSAGESTORE_HH

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "Common.hh"

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

    // Returns a specific message from a given host. Returns NULL if the
    // specified message does not exist. Caller is responsible for deleting
    // the returned QString.
    QString* getMessage(QString& hostName, int messsageNum);

    // Records a new message in internal data structures.
    // Returns true if the message is new, false if not.
    bool recordMessage(MessageInfo& mesInf);

signals:
    // Indicates that a new message has been seen for the first time. mesInf
    // is that message.
    void newMessage(MessageInfo& mesInf);

private:
    // Map keyed by host names.
    // The value QMaps are keyed by message number and contain
    // QString messages.
    QMap<QString, QMap<int, QString> > m_messages;
};

extern MessageStore* GlobalMessages;

#endif // MESSAGESTORE_HH
