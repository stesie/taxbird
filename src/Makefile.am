## Process this file with automake to produce Makefile.in

SUBDIRS = guile

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DVERSION=\""$(VERSION)"\" \
	@PACKAGE_CFLAGS@ 

bin_PROGRAMS = taxbird
bin_SCRIPTS = taxbird-print-helper.sh

taxbird_SOURCES = \
        main.c \
	console.c console.h \
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
	taxbird-print-helper.sh \
	taxbird-sig-update.sh \
	taxbird-sig-makecert.sh

BUILT_SOURCES = interface.c interface.h \
	support.c support.h

MAINTAINER_CLEAN =

support.h: support.c
support.c: interface.c

interface.h: interface.c
interface.c: ../taxbird.glade
	glade-2 -w ../taxbird.glade

