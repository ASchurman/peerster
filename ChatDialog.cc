#include <QVBoxLayout>
#include <QDebug>

#include "ChatDialog.hh"
#include "NetSocket.hh"

ChatDialog* GlobalChatDialog;

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

    m_pNeighbor = new QLineEdit(this);

    // Lay out the widgets to appear in the main window.
    // For Qt widget and layout concepts see:
    // http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_pChatView);
    layout->addWidget(m_pMessageBox);
    layout->addWidget(m_pNeighbor);
    setLayout(layout);

    // Register a callback on the m_pMessageBox's textChanged signal
    // so that we can send the message entered by the user.
    connect(m_pMessageBox, SIGNAL(textChanged()),
            this, SLOT(gotTextChanged()));

    connect(m_pNeighbor, SIGNAL(returnPressed()),
            this, SLOT(processNeighborLine()));
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

        GlobalSocket->inputMessage(message);

        // Clear the m_pMessageBox to get ready for the next input message.
        m_pMessageBox->clear();
    }
}

void ChatDialog::printMessage(MessageInfo& mesInf)
{
    m_pChatView->append(mesInf.m_body);
}

void ChatDialog::processNeighborLine()
{
    QString str = m_pNeighbor->displayText();
    GlobalSocket->addNeighbor(str);
    m_pNeighbor->clear();
}
