#ifndef CHATDIALOG_HH
#define CHATDIALOG_HH

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>

#include "Common.hh"

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    ChatDialog();

public slots:
    void gotTextChanged();
    void printMessage(MessageInfo& mesInf);
    void processNeighborLine();

private:
    QTextEdit* m_pChatView;
    QTextEdit* m_pMessageBox;

    QLineEdit* m_pNeighbor;
};

extern ChatDialog* GlobalChatDialog;

#endif // CHATDIALOG_HH
