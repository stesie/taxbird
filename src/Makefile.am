## Process this file with automake to produce Makefile.in

SUBDIRS = guile

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DVERSION=\""$(VERSION)"\" \
	@GNOME_CFLAGS@ @GEIER_CFLAGS@ @GUILE_CFLAGS@

bin_PROGRAMS = taxbird
bin_SCRIPTS = taxbird-print-helper

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
	export.c export.h

taxbird_LDADD = @GNOME_LIBS@ @GEIER_LIBS@ $(INTLLIBS) @GUILE_LDFLAGS@

EXTRA_DIST = \
	taxbird-print-helper

BUILT_SOURCES = interface.c interface.h \
	support.c support.h

MAINTAINER_CLEAN =

support.h: support.c
support.c: interface.c

interface.h: interface.c
interface.c: ../taxbird.glade
	glade-2 -w ../taxbird.glade

