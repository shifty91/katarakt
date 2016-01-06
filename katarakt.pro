TEMPLATE = app
TARGET = katarakt
DEPENDPATH += .
INCLUDEPATH += .
CONFIG += qt
QT += network xml dbus

unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += poppler-qt4

    isEmpty(PKG_CONFIG):PKG_CONFIG = pkg-config    # same as in link_pkgconfig.prf
    POPPLER_VERSION = $$system($$PKG_CONFIG --modversion poppler-qt4)
    POPPLER_VERSION_MAJOR = $$system(echo "$$POPPLER_VERSION" | cut -d . -f 1)
    POPPLER_VERSION_MINOR = $$system(echo "$$POPPLER_VERSION" | cut -d . -f 2)
    POPPLER_VERSION_MICRO = $$system(echo "$$POPPLER_VERSION" | cut -d . -f 3)

    DEFINES += POPPLER_VERSION_MAJOR=$$POPPLER_VERSION_MAJOR
    DEFINES += POPPLER_VERSION_MINOR=$$POPPLER_VERSION_MINOR
    DEFINES += POPPLER_VERSION_MICRO=$$POPPLER_VERSION_MICRO
}

QMAKE_CXXFLAGS_DEBUG += -DDEBUG

# Input
HEADERS +=  src/layout/layout.h src/layout/singlelayout.h src/layout/gridlayout.h src/layout/presenterlayout.h \
            src/viewer.h src/canvas.h src/resourcemanager.h src/grid.h src/search.h src/gotoline.h src/config.h \
            src/download.h src/util.h src/kpage.h src/worker.h src/beamerwindow.h src/toc.h src/splitter.h src/selection.h \
            src/dbus/source_correlate.h src/dbus/dbus.h

SOURCES +=  src/main.cpp \
            src/layout/layout.cpp src/layout/singlelayout.cpp src/layout/gridlayout.cpp src/layout/presenterlayout.cpp \
            src/viewer.cpp src/canvas.cpp src/resourcemanager.cpp src/grid.cpp src/search.cpp src/gotoline.cpp src/config.cpp \
            src/download.cpp src/util.cpp src/kpage.cpp src/worker.cpp src/beamerwindow.cpp src/toc.cpp src/splitter.cpp \
            src/selection.cpp src/dbus/source_correlate.cpp src/dbus/dbus.cpp

documentation.target = doc/katarakt.1
documentation.depends = doc/katarakt.txt
documentation.commands = a2x -f manpage doc/katarakt.txt

doc.depends = $$documentation.target
doc.CONFIG = phony

website.target = www/index.html
website.depends = www/index.txt
website.commands = asciidoc www/index.txt

web.depends = $$website.target
web.CONFIG = phony

QMAKE_EXTRA_TARGETS += documentation website doc web
