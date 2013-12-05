#ifndef TRUSTCHALLENGE_HH
#define TRUSTCHALLENGE_HH

#include <QString>

// TODO TrustChallenge

// Contains state for a single trust challenge initiated by me
class TrustChallenge
{
public:
    TrustChallenge();

    // User name that we're challenging
    QString m_name;
};

#endif // TRUSTCHALLENGE_HH
