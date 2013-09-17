#ifndef ROUTE_TABLE_HH
#define ROUTE_TABLE_HH

#include <QObject>
#include <QHash>

#include "Common.hh"

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
    void addRoute(MessageInfo& mesInf, AddrInfo& addr);

private:
    // Contains routing info. Keyed by ORIGIN value. Values are the next hop
    // to reach that ORIGIN.
    QHash<QString, AddrInfo> m_table;
};

extern RouteTable* GlobalRoutes;

#endif // ROUTE_TABLE_HH
