#include <time.h>

#include <QApplication>
#include <QDebug>
#include <QtCrypto>

#include "ChatDialog.hh"
#include "NetSocket.hh"
#include "MessageStore.hh"
#include "RouteTable.hh"
#include "messageinfo.hh"
#include "addrinfo.hh"
#include "FileStore.hh"
#include "finalProject/crypto.hh"

int main(int argc, char **argv)
{
    QCA::Initializer qcainit;

    // Initialize rand
    srand(time(NULL));

    // Initialize Qt toolkit
    QApplication app(argc,argv);

    // Create some global objects
    GlobalSocket = new NetSocket();
    GlobalChatDialog = new ChatDialog();
    GlobalMessages = new MessageStore();
    GlobalRoutes = new RouteTable();
    GlobalFiles = new FileStore();
    GlobalCrypto = new Crypto();

    // Create an initial chat dialog window
    GlobalChatDialog->show();

    // Create a UDP network socket
    if (!GlobalSocket->bind())
    {
        exit(1);
    }

    // Connect the signal from GlobalMessages indicating that we received a new
    // message for the first time to the slot in GlobalChatDialog that prints
    // a new message
    QObject::connect(GlobalMessages, SIGNAL(newMessage(MessageInfo&, AddrInfo&, bool)),
                     GlobalChatDialog, SLOT(printMessage(MessageInfo&)));

    // Connect the signal from GlobalMessages indicating that we received a new
    // message for the first time to the slot in GlobalRoutes that records
    // an entry in the routing table.
    QObject::connect(GlobalMessages, SIGNAL(newMessage(MessageInfo&, AddrInfo&, bool)),
                     GlobalRoutes, SLOT(addRoute(MessageInfo&, AddrInfo&, bool)));

    // Parse command line arguments
    QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.count(); i++)
    {
        if (args[i] == "-noforward")
        {
            GlobalSocket->noForward();
        }
        else if (args[i] == "-badsigning")
        {
            GlobalCrypto->setBadSig();
        }
        else if (args[i] == "-badencryption")
        {
            GlobalCrypto->setBadCrypto();
        }
        else
        {
            GlobalSocket->addNeighbor(args[i]);
        }
    }

    // send route rumor message to "prime the pump"
    GlobalSocket->sendRandRouteRumor();

    // Enter the Qt main loop; everything else is event driven
    return app.exec();
}

