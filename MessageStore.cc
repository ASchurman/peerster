#include <QDebug>

#include "MessageStore.hh"

MessageStore* GlobalMessages;

QVariantMap* MessageStore::getStatus()
{
    QVariantMap* pStatus = new QVariantMap();

    QList<QString> hosts = m_messages.keys();
    for (int i = 0; i < hosts.count(); i++)
    {
        QMap<int, QString> hostMap = m_messages[hosts[i]];
        QList<int> mesNums = hostMap.keys();
        int lastMes = mesNums.last() + 1;
        QVariant lastMesVariant(lastMes);
        pStatus->insert(hosts[i], lastMesVariant);
    }

    return pStatus;
}

int MessageStore::getStatusDiff(QVariantMap& remoteStatus, MessageInfo& mesInfOut)
{
    QVariantMap* status = getStatus();

    // True if the remote host has messages that we don't have.
    bool remoteHasExtra = false;

    QList<QString> hosts = status->keys();
    for (int i = 0; i < hosts.count(); i++)
    {
        QString hostName = hosts[i];

        bool remoteMissing = !remoteStatus.contains(hostName);
        int remoteNeed = remoteMissing ? 1 : remoteStatus[hostName].toInt();
        int localNeed = (*status)[hostName].toInt();

        if (remoteMissing || remoteNeed < localNeed)
        {
            // we have messages from a host that they don't!
            mesInfOut.m_host = hostName;

            QList<int> seqNos = m_messages[hostName].keys();
            QList<QString> messes = m_messages[hostName].values();
            for (int j = 0; j < seqNos.count(); j++)
            {
                if (remoteMissing || seqNos[j] >= remoteNeed)
                {
                    mesInfOut.m_seqNo = seqNos[j];
                    mesInfOut.m_body = messes[j];
                    return 1;
                }
            }
            qDebug() << "Our status is constructed incorrectly!";
        }
        else if (remoteNeed > localNeed)
        {
            remoteHasExtra = true;
        }
    }

    if (remoteHasExtra || remoteStatus.count() > status->count())
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

QString* MessageStore::getMessage(QString& hostName, int messsageNum)
{
    if (m_messages.contains(hostName)
        && m_messages[hostName].contains(messsageNum))
    {
        QString* pMessage = new QString(m_messages[hostName][messsageNum]);
        return pMessage;
    }
    else
    {
        return NULL;
    }
}

bool MessageStore::recordMessage(MessageInfo& mesInf)
{
    QString& hostName = mesInf.m_host;
    QString& message = mesInf.m_body;
    int num = mesInf.m_seqNo;

    if (m_messages.contains(hostName))
    {
        if (!m_messages[hostName].contains(num))
        {
            m_messages[hostName].insert(num, message);
            emit newMessage(mesInf);
            qDebug() << "Got new message. Host: " << hostName
                << ", seqno: " << num;
            return true;
        }
    }
    else
    {
        QMap<int, QString> newHostMap;
        newHostMap.insert(num, message);
        m_messages.insert(hostName, newHostMap);
        emit newMessage(mesInf);
        qDebug() << "Got new message (new host). Host: " << hostName
            << ", seqno: " << num;
        return true;
    }

    return false;
}
