#include "addrinfo.hh"

bool AddrInfo::operator==(AddrInfo addrInfo)
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
