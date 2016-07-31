#ifndef SERVER_H
#define SERVER_H

#include <QImage>
#include <QHostAddress>

class QUdpSocket;

class Server : public QObject {
	Q_OBJECT
public:
	Server(QObject *parent = 0, QHostAddress host = QHostAddress::Any, quint64 port = 8080);
	~Server();

	void setNumLEDsPerStrip(int num);
	void setStrips(int num);
	void listen();

public slots:
	void onImageAvailable(QImage image);
	void onReadyRead();

private:
	QHostAddress host;
	quint16 port;
	QImage *mImage;
	QUdpSocket *server;
	int numLEDsPerStrip;
	int numStrips;

	void reply(QByteArray, QHostAddress sender, quint16 senderPort);
};

#endif
