#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDebug>
#include <QInputDialog>
#include <QByteArray>

#include "ChatDialog.hh"
#include "NetSocket.hh"
#include "FileStore.hh"

#define BROADCAST "Send to All"

ChatDialog* GlobalChatDialog;

ChatDialog::ChatDialog()
{
    setWindowTitle("Peerster");

    m_pSearch = NULL;

    // Read-only text box where we display messages from everyone.
    // This widget expands both horizontally and vertically.
    m_pChatView = new QTextEdit(this);
    m_pChatView->setReadOnly(true);

    // Small text-entry box the user can enter messages.
    m_pMessageBox = new QTextEdit(this);
    m_pMessageBox->setFocus();
    QFontMetrics font(m_pMessageBox->font());
    int rowHeight = font.lineSpacing();
    m_pMessageBox->setFixedHeight(5 * rowHeight + 5);

    // a line in which the user can add new neighbors
    m_pNeighbor = new QLineEdit(this);
    m_pNeighbor->setPlaceholderText("Add neighbor...");

    // List of origins to send a private message to, with an option to send to
    // everyone using the gossip protocol
    m_pSendOptions = new QListWidget();
    m_pSendOptions->addItem(BROADCAST);
    m_pSendOptions->setCurrentRow(0);

    // button to open file dialog for sharing a new file
    m_pShareFileButton = new QPushButton("Share File...", this);

    // button to download a file
    m_pDownloadFileButton = new QPushButton("Download File from Selected Peer...", this);

    // button to search for a file
    m_pSearchFileButton = new QPushButton("Search for File...", this);

    m_pSearchResults = new QListWidget();

    m_pCancelSearchButton = new QPushButton("Clear Search");

    // List of all files currently being shared by this node
    m_pSharedFiles = new QListWidget();

    // top-level layout that contains child layouts
    QHBoxLayout* topLayout = new QHBoxLayout();

    // layout for the left side of the window, containing the chat textbox
    // and the widgets to add new neighbors
    m_pChatLayout = new QVBoxLayout();
    m_pChatLayout->addWidget(m_pNeighbor);
    m_pChatLayout->addWidget(m_pChatView);

    // layout for the middle of the window, containing the list of nodes
    // to which private messages can be sent and the textbox in which messages
    // are composed
    m_pSendLayout = new QVBoxLayout();
    m_pSendLayout->addWidget(m_pSendOptions);
    m_pSendLayout->addWidget(m_pDownloadFileButton);
    m_pSendLayout->addWidget(m_pMessageBox);

    // layout for the right side of the window, containing widgets for file-
    // sharing
    m_pFileLayout = new QVBoxLayout();
    m_pSharedFileLayout = new QVBoxLayout();
    m_pSharedFileBox = new QGroupBox("Shared Files");
    m_pSharedFileLayout->addWidget(m_pSharedFiles);
    m_pSharedFileLayout->addWidget(m_pShareFileButton);
    m_pSharedFileBox->setLayout(m_pSharedFileLayout);
    m_pSearchLayout = new QVBoxLayout();
    m_pSearchBox = new QGroupBox("Search Results");
    m_pSearchLayout->addWidget(m_pSearchResults);
    m_pSearchLayout->addWidget(m_pSearchFileButton);
    m_pSearchLayout->addWidget(m_pCancelSearchButton);
    m_pSearchBox->setLayout(m_pSearchLayout);
    m_pFileLayout->addWidget(m_pSharedFileBox);
    m_pFileLayout->addWidget(m_pSearchBox);

    // Add layouts to top-level layout
    topLayout->addLayout(m_pChatLayout);
    topLayout->addLayout(m_pSendLayout);
    topLayout->addLayout(m_pFileLayout);
    setLayout(topLayout);

    // Register a callback on the m_pMessageBox's textChanged signal
    // so that we can send the message entered by the user.
    connect(m_pMessageBox, SIGNAL(textChanged()),
            this, SLOT(gotTextChanged()));

    // When return is pressed in m_pNeighbor, we call processNeighborLine to
    // register the new neighbor
    connect(m_pNeighbor, SIGNAL(returnPressed()),
            this, SLOT(processNeighborLine()));

    // Connect share file button signal to function to show the share file
    // dialog
    connect(m_pShareFileButton, SIGNAL(clicked()),
            this, SLOT(showShareFileDialog()));

    connect(m_pDownloadFileButton, SIGNAL(clicked()),
            this, SLOT(newDownloadFile()));

    connect(m_pSearchFileButton, SIGNAL(clicked()),
            this, SLOT(searchForFile()));

    connect(m_pCancelSearchButton, SIGNAL(clicked()),
            this, SLOT(cancelSearch()));

    connect(m_pSearchResults, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(searchResultDoubleClicked(QListWidgetItem*)));

    resize(750, 450);
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
        // INTENTIONAL SINGLE =
        if ( (pSendOption = m_pSendOptions->currentItem()) )
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
        QString seqNo;
        seqNo.setNum(mesInf.m_seqNo);

        QString chatText("[");
        chatText.append(mesInf.m_host);
        chatText.append(", ");
        chatText.append(seqNo);
        chatText.append("] ");
        chatText.append(mesInf.m_body);

        m_pChatView->append(chatText);
    }
}

void ChatDialog::printPrivate(QString& chatText)
{
    QString sender("Unknown");
    printPrivate(chatText, sender);
}

void ChatDialog::printPrivate(QString& chatText, QString& sender)
{
    QString text("[");
    text.append(sender);
    text.append(", priv] ");
    text.append(chatText);

    m_pChatView->append(text);
}

void ChatDialog::printPrivate(PrivateChat& priv)
{
    if (priv.hasOrigin())
    {
        printPrivate(priv.m_text, priv.m_origin);
    }
    else
    {
        printPrivate(priv.m_text);
    }
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

void ChatDialog::showShareFileDialog()
{
    QStringList shareFiles = QFileDialog::getOpenFileNames(
                                 this,
                                 "Share Files");

    for (int i = 0; i < shareFiles.size(); i++)
    {
        qDebug() << "Sharing: " << shareFiles[i];

        if (GlobalFiles->addSharingFile(shareFiles[i]))
        {
            QStringList strList = shareFiles[i].split('/');
            m_pSharedFiles->addItem(strList.last());
        }
    }
}

void ChatDialog::newDownloadFile()
{
    QString host = m_pSendOptions->currentItem()->text();
    if (host == BROADCAST)
    {
        qDebug() << "Select a peer from whom to download; cannot download from 'Send to All'";
        return;
    }
    else if (host == GlobalSocket->m_hostName)
    {
        qDebug() << "Cannot download a file from self";
        return;
    }

    QString fileName = saveFileString();
    if (fileName.isEmpty()) return;

    QString hashStr = QInputDialog::getText(this,
                                            "File ID",
                                            "Enter the SHA-256 hash of the blocklist:");
    if (hashStr.isEmpty()) return;
    QByteArray hash = QByteArray::fromHex(hashStr.toUtf8());

    qDebug() << "Host: " << host;
    qDebug() << "Hash: " << hash.toHex();

    GlobalFiles->addDownloadFile(fileName, hash, host);
}

void ChatDialog::searchForFile()
{
    // Check if we have a search already going; only run 1 search at a time
    if (m_pSearch)
    {
        qDebug() << "WE ALREADY HAVE SEARCH RUNNING";
        return;
    }

    // Create Search
    QString searchStr = QInputDialog::getText(this, "Search", "Enter search terms:");
    if (searchStr.isEmpty()) return;

    m_pSearch = new Search(searchStr);

    // Connect GlobalSocket got search result signal to Search
    connect(GlobalSocket, SIGNAL(gotSearchResult(QString&,QString&,QByteArray&,QString&)),
            m_pSearch, SLOT(addResult(QString&,QString&,QByteArray&,QString&)));

    // Connect Search new result signal to this object's slot for printing result
    connect(m_pSearch, SIGNAL(newSearchResult(QString&)),
            this, SLOT(printSearchResult(QString&)));

    m_pSearch->beginSearch();
}

void ChatDialog::cancelSearch()
{
    if (m_pSearch)
    {
        qDebug() << "Canceling search";
        delete m_pSearch;
        m_pSearch = NULL;
        m_pSearchResults->clear();
    }
    else
    {
        qDebug() << "No search in progress";
    }
}

void ChatDialog::printSearchResult(QString& fileName)
{
    m_pSearchResults->addItem(fileName);
}

void ChatDialog::searchResultDoubleClicked(QListWidgetItem *item)
{
    if (!m_pSearch)
    {
        qDebug() << "BUG!!!! m_pSearch is NULL, but list item was double-clicked";
        return;
    }

    QPair<QByteArray, QString> file = m_pSearch->m_results[item->text()];
    QByteArray fileId = file.first;
    QString host = file.second;
    QString fileName = saveFileString();
    GlobalFiles->addDownloadFile(fileName, fileId, host);
}

QString ChatDialog::saveFileString()
{
    QString fileName;
    for(;;)
    {
        fileName = QFileDialog::getSaveFileName(this);
        QFile file(fileName);
        if (file.exists())
        {
            qDebug() << "DON'T OVERWRITE AN EXISTING FILE";
        }
        else
        {
            return fileName;
        }
    }
}
