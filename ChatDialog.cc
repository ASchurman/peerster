#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include "ChatDialog.hh"
#include "NetSocket.hh"

#define BROADCAST "Send to All"

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

    // a line in which the user can add new neighbors
    m_pNeighbor = new QLineEdit(this);

    // List of origins to send a private message to, with an option to send to
    // everyone using the gossip protocol
    m_pSendOptions = new QListWidget();
    m_pSendOptions->addItem(BROADCAST);

    QHBoxLayout* topLayout = new QHBoxLayout();

    m_pChatLayout = new QVBoxLayout();
    m_pChatLayout->addWidget(m_pChatView);
    m_pChatLayout->addWidget(m_pNeighbor);

    m_pSendLayout = new QVBoxLayout();
    m_pSendLayout->addWidget(m_pSendOptions);
    m_pSendLayout->addWidget(m_pMessageBox);

    topLayout->addLayout(m_pChatLayout);
    topLayout->addLayout(m_pSendLayout);
    setLayout(topLayout);

    // Register a callback on the m_pMessageBox's textChanged signal
    // so that we can send the message entered by the user.
    connect(m_pMessageBox, SIGNAL(textChanged()),
            this, SLOT(gotTextChanged()));

    // When return is pressed in m_pNeighbor, we call processNeighborLine to
    // register the new neighbor
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

        QListWidgetItem* pSendOption;
        if (pSendOption = m_pSendOptions->currentItem())
        {
            QString sendOptionStr = pSendOption->text();

            if (sendOptionStr == BROADCAST)
            {
                GlobalSocket->inputMessage(message);
            }
            else
            {
                GlobalSocket->sendPrivate(sendOptionStr, message);
            }
        }
        else
        {
            qDebug() << "No send option selected";
        }

        // Clear the m_pMessageBox to get ready for the next input message.
        m_pMessageBox->clear();
    }
}

void ChatDialog::printMessage(MessageInfo& mesInf)
{
    if (!mesInf.m_isRoute)
    {
        m_pChatView->append(mesInf.m_body);
    }
}

void ChatDialog::printPrivate(QString& chatText)
{
    m_pChatView->append(chatText);
}

void ChatDialog::processNeighborLine()
{
    QString str = m_pNeighbor->displayText();
    GlobalSocket->addNeighbor(str);
    m_pNeighbor->clear();
}

void ChatDialog::addOriginForPrivates(QString& host)
{
    m_pSendOptions->addItem(host);
}
