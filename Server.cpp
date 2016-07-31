#include "Server.h"

#include <QUdpSocket>
#include <QtMath>

static const int NUM_CHANNELS = 3;

Server::Server(QObject *parent, QHostAddress host, quint64 port) :
	QObject(parent),
	host(host),
	port(port),
	mImage(NULL),
	server(new QUdpSocket(this)),
	numLEDsPerStrip(0),
	numStrips(0)
{
}

Server::~Server()
{
	if (mImage)
		delete mImage;

	if (server)
		delete server;
}

void Server::onImageAvailable(QImage newImage)
{
	if (mImage)
		delete mImage;

	mImage = new QImage(newImage);
}

void Server::onReadyRead()
{
	while (server->hasPendingDatagrams()) {
		QByteArray datagram;
		QHostAddress sender;
		quint16 senderPort;

		datagram.resize(server->pendingDatagramSize());
		server->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
		reply(datagram, sender, senderPort);
	}
}

void Server::reply(QByteArray datagram, QHostAddress sender, quint16 senderPort)
{
	char pixelData[numLEDsPerStrip * numStrips * NUM_CHANNELS];
	float angle = 0;

	if (!mImage || (mImage->isNull() || numLEDsPerStrip <= 0 || numStrips <= 0))
		return;

	angle = *((float *) datagram.data());

	const float STRIP_GAP_ANGLE = 2 * M_PI / numStrips;
	const float cx = mImage->width() / 2;
	const float cy = mImage->height() / 2;
	const float radius = cx > cy ? cy : cx;

	for (int i = 0; i < numStrips; i++) {
		const float dx = radius / numLEDsPerStrip * qCos(angle + i * STRIP_GAP_ANGLE);
		const float dy = radius / numLEDsPerStrip * qSin(angle + i * STRIP_GAP_ANGLE);
		float x = cx;
		float y = cy;

		for (int j = 0; j < numLEDsPerStrip; j++) {
			const unsigned int argb = mImage->pixel(x, y);
			const unsigned int index = (i * numLEDsPerStrip + j) * NUM_CHANNELS;
			pixelData[index + 0] = qRed(argb);
			pixelData[index + 1] = qGreen(argb);
			pixelData[index + 2] = qBlue(argb);
			x += dx;
			y += dy;
		}
	}

	server->writeDatagram(pixelData, numLEDsPerStrip * numStrips * NUM_CHANNELS, sender, senderPort);
	server->flush();
	fprintf(stdout, "sent %d bytes\n", numLEDsPerStrip * numStrips * NUM_CHANNELS);
}

void Server::setNumLEDsPerStrip(int num)
{
	numLEDsPerStrip = num;
}

void Server::setStrips(int num)
{
	numStrips = num;
}

void Server::listen()
{
	if (server) {
		server->bind(host, port);
		connect(server, &QIODevice::readyRead, this, &Server::onReadyRead);
	}
}
