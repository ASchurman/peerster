#ifndef SEARCH_HH
#define SEARCH_HH

#include <QObject>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QTimer>
#include <QPair>

class Search : public QObject
{
    Q_OBJECT

public:
    Search(QString& terms);

    // Begins search by starting the broadcast timer
    void beginSearch();

    // The search string. Space-separated list of search terms.
    QString m_terms;

    // Contains fileIDs of search results and the hosts they came from.
    // Keyed by filename
    QHash<QString, QPair<QByteArray, QString> > m_results;

public slots:
    // Executes the search by sending out messages to peers. Makes call to
    // GlobalSocket.
    void execute();

    // Adds a search result that we received from a peer
    void addResult(QString& terms, QString& fileName, QByteArray& hash, QString& host);

signals:
    void newSearchResult(QString& fileName);

private:
    // Rebroadcast budget for the next broadcast (thus it is increased after
    // a broadcast)
    int m_budget;

    QTimer* m_timer;
};

#endif // SEARCH_HH
