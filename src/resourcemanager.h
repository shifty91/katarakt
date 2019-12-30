#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QFileSystemWatcher>
#if QT_VERSION >= 0x050000
#	include <poppler-qt5.h>
#else
#	include <poppler-qt4.h>
#endif
#include <list>
#include <set>


class ResourceManager;
class Canvas;
class KPage;
class Worker;
class Viewer;
class QSocketNotifier;
class QDomDocument;
class SelectionLine;


class Request {
public:
	Request(int width, int index);

	int get_lowest_index();
	bool has_index(int index);
	bool remove_index_ok(int index);
	void update(int width, int index);

	int width[3];
};


class ResourceManager : public QObject {
	Q_OBJECT

public:
	ResourceManager(const QString &file, Viewer *v);
	~ResourceManager();

	void load(const QString &file, const QByteArray &password);

	// document opened correctly?
	bool is_valid() const;
	bool is_locked() const;

	const QString &get_file() const;
	void set_file(const QString &new_file);
	// page (meta)data
	const KPage *get_page(int page, int newWidth, int index);
//	QString get_page_label(int page) const;
	float get_page_width(int page, bool rotated = true) const;
	float get_page_height(int page, bool rotated = true) const;
	float get_page_aspect(int page, bool rotated = true) const;
	float get_min_aspect(bool rotated = true) const;
	float get_max_aspect(bool rotated = true) const;
	int get_page_count() const;
	const QList<Poppler::Link *> *get_links(int page);
	const QList<SelectionLine *> *get_text(int page);
	QDomDocument *get_toc() const;

	int get_rotation() const;
	void rotate(int value, bool relative = true);
	void unlock_page(int page) const;
	void invert_colors();
	bool are_colors_inverted() const;

	void collect_garbage(int keep_min, int keep_max, int index);

	void connect_canvas() const;

	void store_jump(int page);
	void clear_jumps();
	int jump_back();
	int jump_forward();

	Poppler::LinkDestination *resolve_link_destination(const QString &name) const;

public slots:
	void file_modified(const QString &path);

private:
	void enqueue(int page, int width, int index = 0);

	void initialize(const QString &file, const QByteArray &password);
	void join_threads();
	void shutdown();

	// sadly, poppler's renderToImage only supports one thread per document
	Worker *worker;

	Viewer *viewer;

	QString file;
	Poppler::Document *doc;
	QMutex requestMutex;
	QMutex garbageMutex;
	QSemaphore requestSemaphore;
	int center_page;
	float max_aspect;
	float min_aspect;
	std::map<int, Request> requests; // page, index, width
	std::set<int> garbage[3];
	QMutex link_mutex;

	KPage *k_page;

	friend class Worker;

	int page_count;
	int rotation;

	QFileSystemWatcher *file_system_watcher;

	bool inverted_colors;

	std::list<int> jumplist;
	std::map<int,std::list<int>::iterator> jump_map;
	std::list<int>::iterator cur_jump_pos;
};

#endif

