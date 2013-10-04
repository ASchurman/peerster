#include <QDebug>

#include "MessageStore.hh"

MessageStore* GlobalMessages;

QVariantMap* MessageStore::getStatus()
{
    QVariantMap* pStatus = new QVariantMap();

    QList<QString> hosts = m_messages.keys();
    for (int i = 0; i < hosts.count(); i++)
    {
        QMap<int, MessageInfo> hostMap = m_messages[hosts[i]];
        QList<int> mesNums = hostMap.keys();

        // figure out the first seqno for this host that we need
        int firstNeed = 1;
        for (int j = 0; j < mesNums.count(); j++)
        {
            if (mesNums[j] == firstNeed)
            {
                firstNeed++;
            }
        }
        QVariant firstNeedVariant(firstNeed);
        
        pStatus->insert(hosts[i], firstNeedVariant);
    }

    return pStatus;
}

int MessageStore::getStatusDiff(QVariantMap& remoteStatus, MessageInfo& mesInfOut)
{
    QVariantMap* pStatus = getStatus();
    QVariantMap status(*pStatus);
    delete pStatus;

    // True if the remote host has messages that we don't have.
    bool remoteHasExtra = false;

    QList<QString> hosts = status.keys();
    for (int i = 0; i < hosts.count(); i++)
    {
        QString hostName = hosts[i];

        // true if the neighbor doesn't have any messages from this host
        bool remoteMissing = !remoteStatus.contains(hostName);

        // the first seqno that the neighbor needs from this host
        int remoteNeed = remoteMissing ? 1 : remoteStatus[hostName].toInt();

        // the first seqno that we need from this host
        int localNeed = status[hostName].toInt();

        if (remoteMissing || remoteNeed < localNeed)
        {
            // we have messages from a host that they don't!
            mesInfOut.m_host = hostName;

            QList<int> seqNos = m_messages[hostName].keys();
            QList<MessageInfo> messes = m_messages[hostName].values();
            for (int j = 0; j < seqNos.count(); j++)
            {
                if (remoteMissing || seqNos[j] == remoteNeed)
                {
                    mesInfOut.m_seqNo = messes[j].m_seqNo;
                    mesInfOut.m_isRoute = messes[j].m_isRoute;
                    if (!messes[j].m_isRoute) mesInfOut.m_body = messes[j].m_body;
                    return 1;
                }
            }

            // We determined based on statuses that we have messages that the
            // neighbor doesn't have, but we were unable to find one such
            // specific message.
            qDebug() << "Our status is constructed incorrectly!";
        }
        else if (remoteNeed > localNeed)
        {
            remoteHasExtra = true;
        }
    }

    if (remoteHasExtra || remoteStatus.count() > status.count())
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

bool MessageStore::recordMessage(MessageInfo& mesInf, AddrInfo& addr, bool isDirect)
{
    QString& hostName = mesInf.m_host;
    int num = mesInf.m_seqNo;

    if (m_messages.contains(hostName))
    {
        if (!m_messages[hostName].contains(num))
        {
            m_messages[hostName].insert(num, mesInf);
            emit newMessage(mesInf, addr, isDirect);

            if (mesInf.m_isRoute)
            {
                qDebug() << "Got new route message. Host: " << hostName
                    << ", seqno: " << num;
            }
            else
            {
                qDebug() << "Got new message. Host: " << hostName
                    << ", seqno: " << num;
            }
            return true;
        }
    }
    else
    {
        QMap<int, MessageInfo> newHostMap;
        newHostMap.insert(num, mesInf);
        m_messages.insert(hostName, newHostMap);
        emit newMessage(mesInf, addr, isDirect);

        if (mesInf.m_isRoute)
        {
            qDebug() << "Got new route message (new host). Host: " << hostName
                << ", seqno: " << num;
        }
        else
        {
            qDebug() << "Got new message (new host). Host: " << hostName
                << ", seqno: " << num;
        }
        return true;
    }

    return false;
}
