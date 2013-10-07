#include <QDebug>

#include "Search.hh"
#include "NetSocket.hh"

#define MAX_RESULTS (10)
#define INIT_BUDGET (2)
#define MAX_BUDGET (100)

Search::Search(QString& terms)
{
    m_terms = terms;
    m_budget = INIT_BUDGET;
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(execute()));
    m_timer->setInterval(1000);
}

void Search::beginSearch()
{
    m_timer->start();
}

void Search::addResult(QString& terms, QString &fileName, QByteArray &hash, QString &host)
{
    // Make sure this is a result for the search we're currently doing
    if (terms != m_terms)
    {
        qDebug() << "Got search result for wrong search terms";
        return;
    }

    // If this is a new file, add it to result table and emit newSearchResult
    if (!m_results.contains(hash))
    {
        qDebug() << "GOT NEW SEARCH RESULT: " << fileName;
        m_results.insert(hash, host);
        QString hashHex(hash.toHex());
        emit newSearchResult(fileName, host, hashHex);

        // Stop broadcasting if we've gotten to MAX_RESULTS
        if (m_results.count() >= MAX_RESULTS)
        {
            m_timer->stop();
            qDebug() << "Reached max results for search: " << m_terms;
        }
    }
    else
    {
        qDebug() << "Got repeat of search result: " << fileName;
    }
}

void Search::execute()
{
    // Don't broadcast if our results are full or the budget is too big
    if (m_results.count() >= MAX_RESULTS || m_budget > MAX_BUDGET) return;

    qDebug() << "SENDING NEW SEARCH REQUEST: " << m_terms << ", budget: " << m_budget;
    GlobalSocket->sendSearchRequest(m_terms, m_budget);
    m_budget *= 2;
}
