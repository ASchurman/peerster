
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>

#include "main.hh"

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

NetSocket::NetSocket()
{
	// Pick a range of four UDP ports to try to allocate by default,
	// computed based on my Unix user ID.
	// This makes it trivial for up to four Peerster instances per user
	// to find each other on the same host,
	// barring UDP port conflicts with other applications
	// (which are quite possible).
	// We use the range from 32768 to 49151 for this purpose.
	myPortMin = 32768 + (getuid() % 4096)*4;
	myPortMax = myPortMin + 3;
}

bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
	for (int p = myPortMin; p <= myPortMax; p++) {
		if (QUdpSocket::bind(p)) {
			qDebug() << "bound to UDP port " << p;
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}

int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an initial chat dialog window
	ChatDialog dialog;
	dialog.show();

	// Create a UDP network socket
	NetSocket sock;
	if (!sock.bind())
		exit(1);

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

