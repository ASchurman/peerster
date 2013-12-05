#ifndef ROUTE_TABLE_HH
#define ROUTE_TABLE_HH

#include <QObject>
#include <QHash>
#include <QPair>

#include "messageinfo.hh"
#include "addrinfo.hh"

class RouteTable : public QObject
{
    Q_OBJECT

public:
    // Finds the address and port of the next hop needed to reach the host with
    // the ORIGIN value dest. Puts this address and port in nextHopOut if it
    // exists; else doesn't touch nextHopOut. Returns true if the address is
    // found; else returns false.
    bool getNextHop(QString& dest, AddrInfo& nextHopOut);

public slots:
    // Adds or edits a route in the routing table
    void addRoute(MessageInfo& mesInf, AddrInfo& addr, bool isDirectHop);

private:
    // Contains routing info. Keyed by ORIGIN value.
    // Values contain the seqNo of the message that gave the most recent
    // route and the next hop.
    QHash<QString, QPair<int, AddrInfo> > m_table;

    // Keyed by ORIGIN value. Indicates whether the route info for the ORIGIN
    // is a direct hop or not.
    QHash<QString, bool> m_directHop;
};

extern RouteTable* GlobalRoutes;

#endif // ROUTE_TABLE_HH
