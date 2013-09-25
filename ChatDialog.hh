#ifndef CHATDIALOG_HH
#define CHATDIALOG_HH

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QListWidget>

#include "Common.hh"

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
    void processNeighborLine();
    void addOriginForPrivates(QString& host);

private:
    QVBoxLayout* m_pChatLayout;
    QVBoxLayout* m_pSendLayout;

    QTextEdit* m_pChatView;
    QLineEdit* m_pNeighbor;

    QListWidget* m_pSendOptions;
    QTextEdit* m_pMessageBox;
};

extern ChatDialog* GlobalChatDialog;

#endif // CHATDIALOG_HH
