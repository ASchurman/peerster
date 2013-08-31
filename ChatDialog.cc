#include <unistd.h>

#include <QVBoxLayout>
#include <QDebug>

#include "ChatDialog.hh"

ChatDialog::ChatDialog()
{
    setWindowTitle("Peerster");

    // Read-only text box where we display messages from everyone.
    // This widget expands both horizontally and vertically.
    textview = new QTextEdit(this);
    textview->setReadOnly(true);

    // Small text-entry box the user can enter messages.
    messageEdit = new QTextEdit(this);
    messageEdit->setFocus();
    QFontMetrics font(messageEdit->font());
    int rowHeight = font.lineSpacing();
    messageEdit->setFixedHeight(3 * rowHeight + 5);

    // Lay out the widgets to appear in the main window.
    // For Qt widget and layout concepts see:
    // http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(textview);
    layout->addWidget(messageEdit);
    setLayout(layout);

    // Register a callback on the messageEdit's textChanged signal
    // so that we can send the message entered by the user.
    connect(messageEdit, SIGNAL(textChanged()),
        this, SLOT(gotTextChanged()));
}

void ChatDialog::gotTextChanged()
{
    QString message = messageEdit->toPlainText();

    // if the messageEdit's text ends in '\n', then the user just hit return,
    // meaning we should send the message
    if (message.endsWith(QChar('\n')))
    {
        // strip the terminating '\n'
        message.truncate(message.size() - 1);

        // Initially, just echo the string locally.
        // Insert some networking code here...
        qDebug() << "FIX: send message to other peers: " << message;
        textview->append(message);

        // Clear the messageEdit to get ready for the next input message.
        messageEdit->clear();
    }
}
