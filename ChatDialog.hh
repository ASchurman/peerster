#ifndef CHATDIALOG_HH
#define CHATDIALOG_HH

#include <QDialog>
#include <QTextEdit>

class ChatDialog : public QDialog
{
    Q_OBJECT

public:
    ChatDialog();

public slots:
    void gotTextChanged();
    void printMessage(QString& message);

private:
    QTextEdit* m_pChatView;
    QTextEdit* m_pMessageBox;
};

extern ChatDialog* GlobalChatDialog;

#endif // CHATDIALOG_HH
