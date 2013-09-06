#include <time.h>

#include <QApplication>
#include <QDebug>

#include "ChatDialog.hh"
#include "NetSocket.hh"
#include "MessageStore.hh"
#include "Common.hh"

int main(int argc, char **argv)
{
    // Initialize rand
    srand(time(NULL));

    // Initialize Qt toolkit
    QApplication app(argc,argv);

    GlobalSocket = new NetSocket();
    GlobalChatDialog = new ChatDialog();
    GlobalMessages = new MessageStore();

    // Create an initial chat dialog window
    GlobalChatDialog->show();

    // Create a UDP network socket
    if (!GlobalSocket->bind())
    {
        exit(1);
    }

    QObject::connect(GlobalMessages, SIGNAL(newMessage(MessageInfo&)),
                     GlobalChatDialog, SLOT(printMessage(MessageInfo&)));

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}
