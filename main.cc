
#include <QApplication>
#include <QDebug>

#include "ChatDialog.hh"
#include "NetSocket.hh"

int main(int argc, char **argv)
{
    // Initialize Qt toolkit
    QApplication app(argc,argv);

    GlobalSocket = new NetSocket();
    GlobalChatDialog = new ChatDialog();

    // Create an initial chat dialog window
    GlobalChatDialog->show();

    // Create a UDP network socket
    if (!GlobalSocket->bind())
    {
        exit(1);
    }

    QObject::connect(GlobalSocket, SIGNAL(messageReceived(QString&)),
                     GlobalChatDialog, SLOT(printMessage(QString&)));

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}

