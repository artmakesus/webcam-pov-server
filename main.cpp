#include <QCoreApplication>
#include <QCamera>
#include <QCameraInfo>
#include <QCommandLineParser>
#include <QImage>
#include <QList>
#include <QStringList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtMath>

#include "CameraFrameGrabber.h"

static const int NUM_CHANNELS = 3;

static QTcpServer server;
static QImage frame;

// Settings
static int numLEDsPerStrip;
static int numStrips;

static void writePixelData(QTcpSocket *socket, float angle)
{
	char pixelData[numLEDsPerStrip * numStrips * NUM_CHANNELS];

	if (frame.isNull() || numLEDsPerStrip <= 0 || numStrips <= 0)
		return;

	const float STRIP_GAP_ANGLE = 2 * M_PI / numStrips;
	const float cx = frame.width() / 2;
	const float cy = frame.height() / 2;
	const float radius = cx > cy ? cy : cx;

	for (int i = 0; i < numStrips; i++) {
		const float dx = radius / numLEDsPerStrip * qCos(angle + i * STRIP_GAP_ANGLE);
		const float dy = radius / numLEDsPerStrip * qSin(angle + i * STRIP_GAP_ANGLE);
		float x = cx;
		float y = cy;

		for (int j = 0; j < numLEDsPerStrip; j++) {
			const unsigned int argb = frame.pixel(x, y);
			pixelData[(i * numLEDsPerStrip + j) * NUM_CHANNELS + 0] = argb & 0x00FF0000;
			pixelData[(i * numLEDsPerStrip + j) * NUM_CHANNELS + 1] = argb & 0x0000FF00;
			pixelData[(i * numLEDsPerStrip + j) * NUM_CHANNELS + 2] = argb & 0x000000FF;
			x += dx;
			y += dy;
		}
	}

	socket->write(pixelData, numLEDsPerStrip * numStrips * NUM_CHANNELS);
	socket->flush();
}

static void onNewConnection()
{
	QTcpSocket *socket;
	float angle;
	unsigned int nread = 0;

	socket = server.nextPendingConnection();
	socket->waitForConnected();

	fprintf(stdout, "client connected\n");

	while (true) {
		if (!socket->waitForReadyRead())
			break;

		if (socket->bytesAvailable() < (int) (sizeof(angle) - nread))
			continue;

		nread += socket->read(((char *) &angle) + nread, sizeof(angle) - nread);
		if (nread >= sizeof(angle)) {
			writePixelData(socket, angle);
			nread -= sizeof(angle);
		}
	}

	fprintf(stdout, "client disconnected\n");

	delete socket;
}

static void onFrameAvailable(QImage newFrame)
{
	frame = newFrame;
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QCommandLineParser parser;
	QCamera camera;
	CameraFrameGrabber grabber;
	quint16 port = 7777;
	bool ok = true;

	QCoreApplication::setApplicationName("webcam-server");
	QCoreApplication::setApplicationVersion("0.0.0");
	parser.setApplicationDescription("Webcam server that serves POV pixel data");

	QCommandLineOption portOption(
			QStringList() << "p" << "port",
			QCoreApplication::translate("main", "set server port"),
			QCoreApplication::translate("main", "PORT"));
	parser.addOption(portOption);

	QCommandLineOption numLEDsPerStripOption(
			"num-leds-per-strip",
			QCoreApplication::translate("main", "set number of LEDs per strip"),
			QCoreApplication::translate("main", "N"));
	parser.addOption(numLEDsPerStripOption);

	QCommandLineOption numStripsOption(
			"num-strips",
			QCoreApplication::translate("main", "set number of strips"),
			QCoreApplication::translate("main", "N"));
	parser.addOption(numStripsOption);

	parser.addHelpOption();
	parser.addVersionOption();
	parser.process(app);

	if (parser.isSet("port")) {
		QString portValue = parser.value(portOption);
		port = portValue.toUShort(&ok);
		if (!ok) {
			fprintf(stderr, "invalid port value\n");
			return -EINVAL;
		}
	}

	if (parser.isSet("num-leds-per-strip")) {
		QString numLEDsPerStripValue = parser.value(numLEDsPerStripOption);
		numLEDsPerStrip = numLEDsPerStripValue.toInt(&ok);
		if (!ok) {
			fprintf(stderr, "invalid num leds per strip value\n");
			return -EINVAL;
		}
	}
	fprintf(stdout, "num LEDs: %d\n", numLEDsPerStrip);

	if (parser.isSet("num-strips")) {
		QString numStripsValue = parser.value(numStripsOption);
		numStrips = numStripsValue.toInt(&ok);
		if (!ok) {
			fprintf(stderr, "invalid num. strips value\n");
			return -EINVAL;
		}
	}
	if (numStrips <= 0)
		numStrips = 1;
	fprintf(stdout, "num strips: %d\n", numStrips);

	QObject::connect(&grabber, &CameraFrameGrabber::frameAvailable, onFrameAvailable);
	camera.setViewfinder(&grabber);
	camera.start();

	server.listen(QHostAddress::Any, port);
	QObject::connect(&server, &QTcpServer::newConnection, onNewConnection);
	fprintf(stdout, "listening at port %hu\n", port);

	return app.exec();
}
