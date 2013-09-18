#include <QDebug>

#include "RouteTable.hh"
#include "ChatDialog.hh"

RouteTable* GlobalRoutes;

bool RouteTable::getNextHop(QString& dest, AddrInfo& nextHopOut)
{
    if (m_table.contains(dest))
    {
        nextHopOut.m_isDns = false;
        nextHopOut.m_addr = m_table[dest].m_addr;
        nextHopOut.m_port = m_table[dest].m_port;
        return true;
    }
    else
    {
        return false;
    }
}

void RouteTable::addRoute(MessageInfo& mesInf, AddrInfo& addr)
{
    if (addr.m_isDns)
    {
        qDebug() << "Attempting to add route with only DNS address";
    }
    else
    {
        qDebug() << "Adding/editing route for " << mesInf.m_host;
    }

    if (!m_table.contains(mesInf.m_host))
    {
        GlobalChatDialog->addOriginForPrivates(mesInf.m_host);
    }

    m_table[mesInf.m_host].m_addr = addr.m_addr;
    m_table[mesInf.m_host].m_port = addr.m_port;
}
