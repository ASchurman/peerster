#include <QDebug>

#include "RouteTable.hh"
#include "ChatDialog.hh"

RouteTable* GlobalRoutes;

bool RouteTable::getNextHop(QString& dest, AddrInfo& nextHopOut)
{
    if (m_table.contains(dest))
    {
        nextHopOut.m_isDns = false;
        nextHopOut.m_addr = m_table[dest].second.m_addr;
        nextHopOut.m_port = m_table[dest].second.m_port;
        return true;
    }
    else
    {
        return false;
    }
}

void RouteTable::addRoute(MessageInfo& mesInf, AddrInfo& addr, bool isDirectHop)
{
    if (addr.m_isDns)
    {
        qDebug() << "Attempting to add route with only DNS address";
    }

    bool update = false;

    // update GUI if we're adding a route for the first time
    if (!m_table.contains(mesInf.m_host))
    {
        qDebug() << "Adding route for " << mesInf.m_host;
        GlobalChatDialog->addOriginForPrivates(mesInf.m_host);

        update = true;
    }
    else if (mesInf.m_seqNo > m_table[mesInf.m_host].first
             || (mesInf.m_seqNo == m_table[mesInf.m_host].first
                 && isDirectHop))
    {
        qDebug() << "Editing route for " << mesInf.m_host;

        update = true;
    }

    if (update)
    {
        m_directHop[mesInf.m_host] = isDirectHop;
        m_table[mesInf.m_host] = qMakePair(mesInf.m_seqNo, addr);
    }
}
