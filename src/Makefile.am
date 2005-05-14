## Process this file with automake to produce Makefile.in

SUBDIRS = guile pubkeys

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DVERSION=\""$(VERSION)"\" \
	@PACKAGE_CFLAGS@ 

bin_SCRIPTS = taxbird-update.sh

bin_PROGRAMS = taxbird

taxbird_SOURCES = \
        main.c \
        support.c support.h \
        interface.c interface.h \
        callbacks.c callbacks.h \
        workspace.c workspace.h \
        guile.c guile.h \
        form.c form.h \
	dialog.c dialog.h \
	sigcheck.c sigcheck.h \
	export.c export.h \
	digest.c digest.h

taxbird_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

EXTRA_DIST = \
	taxbird-sig-update.sh \
	readline.scm \
	taxbird-sig-update.sh

BUILT_SOURCES = interface.c interface.h \
	support.c support.h

MAINTAINER_CLEAN =

support.h: support.c
support.c: interface.c

interface.h: interface.c
interface.c: ../taxbird.glade
	glade-2 -w ../taxbird.glade

