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

private:
    QTextEdit *textview;
    QTextEdit *messageEdit;
};

#endif // CHATDIALOG_HH
