## Process this file with automake to produce Makefile.in

SUBDIRS = guile

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DVERSION=\""$(VERSION)"\" \
	@GNOME_CFLAGS@ @GEIER_CFLAGS@ @GUILE_CFLAGS@

bin_PROGRAMS = taxbird
bin_SCRIPTS = taxbird-print-helper

taxbird_SOURCES = \
        main.c \
        callbacks.c callbacks.h \
        workspace.c workspace.h \
	glade.c glade.h \
        guile.c guile.h \
        form.c form.h \
	dialog.c dialog.h \
	export.c export.h

taxbird_LDADD = @GNOME_LIBS@ @GEIER_LIBS@ $(INTLLIBS) @GUILE_LDFLAGS@

EXTRA_DIST = \
	taxbird-print-helper

MAINTAINER_CLEAN =
