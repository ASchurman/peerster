
#include <QApplication>
#include <QDebug>

#include "ChatDialog.hh"
#include "NetSocket.hh"

int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an initial chat dialog window
	ChatDialog dialog;
	dialog.show();

	// Create a UDP network socket
	if (!GlobalSocket.bind())
	{
		exit(1);
	}

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

