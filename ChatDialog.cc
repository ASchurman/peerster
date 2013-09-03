#include <QVBoxLayout>
#include <QDebug>

#include "ChatDialog.hh"
#include "NetSocket.hh"

ChatDialog::ChatDialog()
{
    setWindowTitle("Peerster");

    // Read-only text box where we display messages from everyone.
    // This widget expands both horizontally and vertically.
    m_pChatView = new QTextEdit(this);
    m_pChatView->setReadOnly(true);

    // Small text-entry box the user can enter messages.
    m_pMessageBox = new QTextEdit(this);
    m_pMessageBox->setFocus();
    QFontMetrics font(m_pMessageBox->font());
    int rowHeight = font.lineSpacing();
    m_pMessageBox->setFixedHeight(3 * rowHeight + 5);

    // Lay out the widgets to appear in the main window.
    // For Qt widget and layout concepts see:
    // http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_pChatView);
    layout->addWidget(m_pMessageBox);
    setLayout(layout);

    // Register a callback on the m_pMessageBox's textChanged signal
    // so that we can send the message entered by the user.
    connect(m_pMessageBox, SIGNAL(textChanged()),
        this, SLOT(gotTextChanged()));

    // Register a callback on GlobalSocket's readyRead signal so that we can
    // append the received message to m_pChatView
    connect(&GlobalSocket, SIGNAL(readyRead()), this, SLOT(gotReadyRead()));
}

void ChatDialog::gotTextChanged()
{
    QString message = m_pMessageBox->toPlainText();

    // if the m_pMessageBox's text ends in '\n', then the user just hit return,
    // meaning we should send the message
    if (message.endsWith(QChar('\n')))
    {
        // strip the terminating '\n'
        message.truncate(message.size() - 1);

        GlobalSocket.send(message);

        // Clear the m_pMessageBox to get ready for the next input message.
        m_pMessageBox->clear();
    }
}

void ChatDialog::gotReadyRead()
{
    QString* message = GlobalSocket.receive();

    if (message)
    {
        m_pChatView->append(*message);
        delete message;
    }
    else
    {
        qDebug() << "Failed to receive datagram";
    }
}
