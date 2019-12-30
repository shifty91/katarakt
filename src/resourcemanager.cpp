#include <iostream>
#include <limits>
#include <cerrno>
#include <unistd.h>
#include <QSocketNotifier>
#include <QFileInfo>
#include "resourcemanager.h"
#include "canvas.h"
#include "util.h"
#include "kpage.h"
#include "worker.h"
#include "viewer.h"
#include "beamerwindow.h"
#include "selection.h"
#include "layout/layout.h"

using namespace std;


Request::Request(int width, int index) {
	for (int i = 0; i < 3; i++) {
		this->width[i] = -1;
	}
	this->width[index] = width;
}

int Request::get_lowest_index() {
	for (int i = 0; i < 3; i++) {
		if (width[i] != -1) {
			return i;
		}
	}
	// will not happen
	return 0;
}

bool Request::has_index(int index) {
	return width[index] != -1;
}

bool Request::remove_index_ok(int index) {
	width[index] = -1;
	for (int i = 0; i < 3; i++) {
		if (width[i] != -1) {
			return true;
		}
	}
	return false;
}

void Request::update(int width, int index) {
	this->width[index] = width;
}


ResourceManager::ResourceManager(const QString &file, Viewer *v) :
		viewer(v),
		file(file),
		doc(NULL),
		center_page(0),
		rotation(0),
		file_system_watcher(NULL),
		inverted_colors(false),
		cur_jump_pos(jumplist.end()) {
	initialize(file, QByteArray());
}

void ResourceManager::initialize(const QString &file, const QByteArray &password) {
	page_count = 0;
	k_page = NULL;

	doc = NULL;
	if (!file.isNull()) {
		doc = Poppler::Document::load(file, QByteArray(), password);
	}

	worker = new Worker(this);
	if (viewer->get_canvas() != NULL) {
		// on first start the canvas has not yet been constructed
		connect(worker, SIGNAL(page_rendered(int)), viewer->get_canvas(), SLOT(page_rendered(int)), Qt::UniqueConnection);
		connect(worker, SIGNAL(page_rendered(int)), viewer->get_beamer(), SLOT(page_rendered(int)), Qt::UniqueConnection);
	}
	worker->start();

	// setup file system watcher
	QFileInfo info(file);
	file_system_watcher = new QFileSystemWatcher(this);
	file_system_watcher->addPath(file);
	QObject::connect(file_system_watcher, SIGNAL(fileChanged(const QString &)),
					 this, SLOT(file_modified(const QString &)));

	if (doc == NULL) {
		// poppler already prints a debug message
//		cerr << "failed to open file" << endl;
		return;
	}
	if (doc->isLocked()) {
		// poppler already prints a debug message
//		cerr << "missing password" << endl;
		return;
	}
	doc->setRenderHint(Poppler::Document::Antialiasing, true);
	doc->setRenderHint(Poppler::Document::TextAntialiasing, true);
	doc->setRenderHint(Poppler::Document::TextHinting, true);
#if POPPLER_VERSION >= POPPLER_VERSION_CHECK(0, 18, 0)
	doc->setRenderHint(Poppler::Document::TextSlightHinting, true);
#endif
#if POPPLER_VERSION >= POPPLER_VERSION_CHECK(0, 22, 0)
//	doc->setRenderHint(Poppler::Document::OverprintPreview, true); // TODO what is this?
#endif
#if POPPLER_VERSION >= POPPLER_VERSION_CHECK(0, 24, 0)
	doc->setRenderHint(Poppler::Document::ThinLineSolid, true); // TODO what's the difference between ThinLineSolid and ThinLineShape?
#endif

	page_count = doc->numPages();

	min_aspect = numeric_limits<float>::max();
	max_aspect = numeric_limits<float>::min();

	k_page = new KPage[get_page_count()];
	for (int i = 0; i < get_page_count(); i++) {
		Poppler::Page *p = doc->page(i);
		if (p == NULL) {
			cerr << "failed to load page " << i << endl;
			continue;
		}
		k_page[i].width = p->pageSizeF().width();
		k_page[i].height = p->pageSizeF().height();

		float aspect = k_page[i].width / k_page[i].height;
		if (aspect < min_aspect) {
			min_aspect = aspect;
		}
		if (aspect > max_aspect) {
			max_aspect = aspect;
		}

//		k_page[i].label = p->label();
//		if (k_page[i].label != QString::number(i + 1)) {
//			cout << i << endl;
//		}
		delete p;
	}
}

ResourceManager::~ResourceManager() {
	shutdown();
}

void ResourceManager::shutdown() {
	if (worker != NULL) {
		join_threads();
	}
	garbageMutex.lock();
	for (int i = 0; i < 3; i++) {
		garbage[i].clear();
	}
	garbageMutex.unlock();
	requests.clear();
	requestSemaphore.acquire(requestSemaphore.available());
	delete doc;
	delete[] k_page;
	delete worker;
	delete file_system_watcher;
}

void ResourceManager::load(const QString &file, const QByteArray &password) {
	shutdown();
	initialize(file, password);
}

bool ResourceManager::is_valid() const {
	return (doc != NULL);
}

bool ResourceManager::is_locked() const {
	if (doc == NULL) {
		return false;
	}
	return doc->isLocked();
}

const QString &ResourceManager::get_file() const {
	return file;
}

void ResourceManager::set_file(const QString &new_file) {
	file = new_file;
}

const KPage *ResourceManager::get_page(int page, int width, int index) {
	if (page < 0 || page >= get_page_count()) {
		return NULL;
	}

	// page not available or wrong size/rotation/color
	k_page[page].mutex.lock();
	bool must_invert_colors = k_page[page].inverted_colors != inverted_colors;
	if (must_invert_colors) {
		k_page[page].toggle_invert_colors();
	}

	if (k_page[page].img[index].isNull() ||
			k_page[page].status[index] != width ||
			k_page[page].rotation[index] != rotation ||
			must_invert_colors) {
		enqueue(page, width, index);
	}

	return &k_page[page];
}

int ResourceManager::get_rotation() const {
	return rotation;
}

void ResourceManager::rotate(int value, bool relative) {
	value %= 4;
	if (relative) {
		rotation = (rotation + value + 4) % 4;
	} else {
		rotation = value;
	}
}

void ResourceManager::unlock_page(int page) const {
	k_page[page].mutex.unlock();
}

void ResourceManager::invert_colors() {
	inverted_colors = !inverted_colors;
}

bool ResourceManager::are_colors_inverted() const {
	return inverted_colors;
}

void ResourceManager::collect_garbage(int keep_min, int keep_max, int index) {
	requestMutex.lock();
	if (index == 0) { // make a separate center_page for each index?
		center_page = (keep_min + keep_max) / 2;
	}
	requestMutex.unlock();
	// free distant pages
	garbageMutex.lock();
	for (set<int>::iterator it = garbage[index].begin(); it != garbage[index].end(); /* empty */) {
		int page = *it;
		if (page >= keep_min && page <= keep_max) {
			++it; // move on
			continue;
		}
		garbage[index].erase(it++); // erase and move on (iterator becomes invalid)
#ifdef DEBUG
		cerr << "    removing page " << page << endl;
#endif
		k_page[page].mutex.lock();
		k_page[page].img[index] = QImage();
		k_page[page].img_other[index] = QImage();
		k_page[page].status[index] = 0;
		k_page[page].rotation[index] = 0;
		k_page[page].mutex.unlock();
	}
	garbageMutex.unlock();

	// keep the request list small
	if (keep_max < keep_min) {
		return;
	}
	requestMutex.lock();
	for (map<int,Request>::iterator it = requests.begin(); it != requests.end(); ) {
		if ((it->first < keep_min || it->first > keep_max) && it->second.has_index(index)) {
			if (!it->second.remove_index_ok(index)) { // no index left in request -> delete
				requestSemaphore.acquire(1);
				requests.erase(it++);
			}
		} else {
			++it;
		}
	}
	requestMutex.unlock();
}

void ResourceManager::connect_canvas() const {
	connect(worker, SIGNAL(page_rendered(int)), viewer->get_canvas(), SLOT(page_rendered(int)), Qt::UniqueConnection);
	connect(worker, SIGNAL(page_rendered(int)), viewer->get_beamer(), SLOT(page_rendered(int)), Qt::UniqueConnection);
}

void ResourceManager::store_jump(int page) {
	map<int,list<int>::iterator>::iterator it = jump_map.find(page);
	if (it != jump_map.end()) {
		jumplist.erase(it->second);
	}
	jumplist.push_back(page);
	jump_map[page] = --jumplist.end();
	cur_jump_pos = jumplist.end();

//	cerr << "jumplist: ";
//	for (list<int>::iterator it = jumplist.begin(); it != jumplist.end(); ++it) {
//		if (it == cur_jump_pos) {
//			cerr << "*";
//		}
//		cerr << *it << " ";
//	}
//	cerr << endl;
}

void ResourceManager::clear_jumps() {
	jumplist.clear();
	jump_map.clear();
	cur_jump_pos = jumplist.end();
}

int ResourceManager::jump_back() {
	if (cur_jump_pos == jumplist.begin()) {
		return -1;
	}
	if (cur_jump_pos == jumplist.end()) {
		store_jump(viewer->get_canvas()->get_layout()->get_page());
		--cur_jump_pos;
	}
	--cur_jump_pos;
	return *cur_jump_pos;
}

int ResourceManager::jump_forward() {
	if (cur_jump_pos == jumplist.end() || cur_jump_pos == --jumplist.end()) {
		return -1;
	}
	++cur_jump_pos;
	return *cur_jump_pos;
}

Poppler::LinkDestination *ResourceManager::resolve_link_destination(const QString &name) const {
	return doc->linkDestination(name);
}

void ResourceManager::file_modified(const QString &path) {
	viewer->reload(false); // don't clamp
}

void ResourceManager::enqueue(int page, int width, int index) {
	requestMutex.lock();
	map<int,Request>::iterator it = requests.find(page);
	if (it == requests.end()) {
		requests.insert(make_pair(page, Request(width, index)));
		requestSemaphore.release(1);
	} else {
		it->second.update(width, index);
	}
	requestMutex.unlock();
}

//QString ResourceManager::get_page_label(int page) const {
//	if (page < 0 || page >= get_page_count()) {
//		return QString();
//	}
//	return k_page[page].label;
//}

float ResourceManager::get_page_width(int page, bool rotated) const {
	if (page < 0 || page >= get_page_count()) {
		return -1;
	}
	if (!rotated || rotation == 0 || rotation == 2) {
		return k_page[page].width;
	}
	// swap if rotated by 90 or 270 degrees
	return k_page[page].height;
}

float ResourceManager::get_page_height(int page, bool rotated) const {
	if (page < 0 || page >= get_page_count()) {
		return -1;
	}
	if (!rotated || rotation == 0 || rotation == 2) {
		return k_page[page].height;
	}
	return k_page[page].width;
}

float ResourceManager::get_page_aspect(int page, bool rotated) const {
	if (page < 0 || page >= get_page_count()) {
		return -1;
	}
	if (!rotated || rotation == 0 || rotation == 2) {
		return k_page[page].width / k_page[page].height;
	}
	return k_page[page].height / k_page[page].width;
}

float ResourceManager::get_min_aspect(bool rotated) const {
	if (!rotated || rotation == 0 || rotation == 2) {
		return min_aspect;
	} else {
		return 1.0f / max_aspect;
	}
}

float ResourceManager::get_max_aspect(bool rotated) const {
	if (!rotated || rotation == 0 || rotation == 2) {
		return max_aspect;
	} else {
		return 1.0f / min_aspect;
	}
}

int ResourceManager::get_page_count() const {
	return page_count;
}

const QList<Poppler::Link *> *ResourceManager::get_links(int page) {
	if (page < 0 || page >= get_page_count()) {
		return NULL;
	}
	link_mutex.lock();
	QList<Poppler::Link *> *l = k_page[page].links;
	link_mutex.unlock();
	return l;
}

const QList<SelectionLine *> *ResourceManager::get_text(int page) {
	if (page < 0 || page >= get_page_count()) {
		return NULL;
	}
	link_mutex.lock();
	QList<SelectionLine *> *t = k_page[page].text;
	link_mutex.unlock();
	return t;
}

QDomDocument *ResourceManager::get_toc() const {
	if (doc == NULL || doc->isLocked()) {
		return NULL;
	}
	return doc->toc();
}

void ResourceManager::join_threads() {
	worker->die = true;
	requestSemaphore.release(1);
	worker->wait();
}

