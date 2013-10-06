#ifndef CHATDIALOG_HH
#define CHATDIALOG_HH

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QGroupBox>

#include "Common.hh"
#include "Search.hh"
#include "PrivateMessage.hh"

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    ChatDialog();

public slots:
    void gotTextChanged();
    void printMessage(MessageInfo& mesInf);
    void printPrivate(QString& chatText);
    void printPrivate(QString& chatText, QString& sender);
    void printPrivate(PrivateChat& priv);
    void processNeighborLine();
    void addOriginForPrivates(QString& host);
    void showShareFileDialog();
    void showShareDirDialog();
    void newDownloadFile();
    void searchForFile();
    void cancelSearch();
    void printSearchResult(QString& fileName);
    void searchResultDoubleClicked(QListWidgetItem* item);

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
    QListWidget* m_pSearchResults;
    QPushButton* m_pSearchFileButton;
    QPushButton* m_pCancelSearchButton;

    QString saveFileString();
};

extern ChatDialog* GlobalChatDialog;

#endif // CHATDIALOG_HH
