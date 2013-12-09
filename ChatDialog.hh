#ifndef CHATDIALOG_HH
#define CHATDIALOG_HH

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QTableWidget>

#include "messageinfo.hh"
#include "Search.hh"
#include "PrivateMessage.hh"

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    ChatDialog();
    QString getChallengeAnswer(const QString& origin, const QString& question);

public slots:
    void gotTextChanged();
    void printMessage(MessageInfo& mesInf);
    void printPrivate(PrivateChat& priv);
    void processNeighborLine();
    void addOriginForPrivates(QString& host);
    void showShareFileDialog();
    void showShareDirDialog();
    void newDownloadFile();
    void searchForFile();
    void cancelSearch();
    void printSearchResult(QString& fileName, QString& origin, QString& hash);
    void searchResultDoubleClicked(int row);
    void repeatSearch();
    void challenge();
    void addTrust(const QString& host);

private:
    Search* m_pSearch;

    QVBoxLayout* m_pChatLayout;
    QVBoxLayout* m_pSendLayout;
    QVBoxLayout* m_pFileLayout;

    QTextEdit* m_pChatView;
    QLineEdit* m_pNeighbor;

    QListWidget* m_pSendOptions;
    QTextEdit* m_pMessageBox;

    QGroupBox* m_pSharedFileBox;
    QGroupBox* m_pSearchBox;
    QVBoxLayout* m_pSharedFileLayout;
    QVBoxLayout* m_pSearchLayout;
    QPushButton* m_pShareFileButton;
    QPushButton* m_pShareDirButton;
    QListWidget* m_pSharedFiles;
    QPushButton* m_pDownloadFileButton;
    QTableWidget* m_pSearchResults;
    QPushButton* m_pSearchFileButton;
    QPushButton* m_pCancelSearchButton;
    QPushButton* m_pRepeatSearchButton;
    QPushButton* m_pChallengeButton;

    QString saveFileString();
    void setSearchResultHeaders();
    void createSearch(QString& terms);
};

extern ChatDialog* GlobalChatDialog;

#endif // CHATDIALOG_HH
