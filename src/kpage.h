#ifndef KPAGE_H
#define KPAGE_H

#include <QImage>
#include <QMutex>
#if POPPLER_QT >= 5
#include <poppler-qt5.h>
#else
#include <poppler-qt4.h>
#endif


class SelectionLine;


class KPage {
private:
	KPage();
	~KPage();

public:
	const QImage *get_image(int index = 0) const;
	int get_width(int index = 0) const;
	char get_rotation(int index = 0) const;
	const QList<SelectionLine *> *get_text() const;
//	QString get_label() const;

private:
	void toggle_invert_colors();

	float width;
	float height;
	QImage img[3];
	QImage thumbnail;
	// for inverted colors with reduced contrast
	// img_other contain the currently not needed color versions
	// img store the current versions to be displayed
	QImage img_other[3];
	QImage thumbnail_other;

//	QString label;
	QList<Poppler::Link *> *links;
	QMutex mutex;
	int status[3];
	char rotation[3];
	bool inverted_colors; // img[]s and thumb must be consistent
	QList<SelectionLine *> *text;

	friend class Worker;
	friend class ResourceManager;
};

#endif

