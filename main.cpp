#include <QApplication>
#include <QString>
#include <QProcess>
#include <iostream>
#include <getopt.h>
#include "download.h"
#include "resourcemanager.h"
#include "viewer.h"
#include "config.h"

using namespace std;


static void print_help(char *name) {
	cout << "Usage:" << endl;
	cout << "  " << name << " [OPTIONS] FILE" << endl;
	cout << endl;
	cout << "Options:" << endl;
	cout << "  -p, --page NUM      Start showing page NUM" << endl;
	cout << "  -f, --fullscreen    Start in fullscreen mode" << endl;
	cout << "  -q, --quit          Quit on initialization failure" << endl;
	cout << "  -h, --help          Print this help and exit" << endl;
}

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	// parse command line options
	struct option long_options[] = {
		{"page",		required_argument,	NULL,	'p'},
		{"fullscreen",	no_argument,		NULL,	'f'},
		{"quit",		no_argument,		NULL,	'q'},
		{"help",		no_argument,		NULL,	'h'},
		{NULL, 0, NULL, 0}
	};
	int c;
	int option_index = 0;
	while (1) {
		c = getopt_long(argc, argv, "+p:fqh", long_options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
			case 'p':
				// currently no warning message on wrong input
				CFG::get_instance()->set_tmp_value("start_page", atoi(optarg) - 1);
				break;
			case 'f':
				CFG::get_instance()->set_tmp_value("fullscreen", true);
				break;
			case 'q':
				CFG::get_instance()->set_tmp_value("quit_on_init_fail", true);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
			default:
				// getopt prints an error message
				return 1;
		}
	}

	// fork more processes if there are arguments left
	if (optind < argc - 1) {
		QStringList l;
		for (int i = optind + 1; i < argc; i++) {
			l << argv[i];
		}
		QProcess::startDetached(argv[0], l);
	}

	QString file;
	Download download;
	if (argv[optind] != NULL) {
		file = download.load(QString::fromUtf8(argv[optind]));
		if (file == NULL) {
			return 1;
		}
	}
	// else no argument given, "open" empty string

	Viewer katarakt(file);
	if (!katarakt.is_valid()) {
		return 1;
	}

	return app.exec();
}

