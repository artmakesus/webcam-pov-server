#include <QCoreApplication>
#include <QCamera>
#include <QCameraInfo>
#include <QCommandLineParser>
#include <QImage>
#include <QList>
#include <QStringList>

#include "CameraFrameGrabber.h"
#include "Server.h"

static CameraFrameGrabber grabber;

// Settings
static int numLEDsPerStrip;
static int numStrips;

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	QCommandLineParser parser;
	QCamera camera;
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

	camera.setViewfinder(&grabber);
	camera.start();

	Server server(NULL, QHostAddress::Any, port);
	server.setNumLEDsPerStrip(numLEDsPerStrip);
	server.setStrips(numStrips);
	QObject::connect(&grabber, &CameraFrameGrabber::frameAvailable, &server, &Server::onImageAvailable);

	server.listen();
	fprintf(stdout, "listening at port %hu\n", port);

	return app.exec();
}
