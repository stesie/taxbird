## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DVERSION=\""$(VERSION)"\" \
	@PACKAGE_CFLAGS@ 

bin_PROGRAMS = taxbird

taxbird_SOURCES = \
        main.c \
        support.c support.h \
        interface.c interface.h \
        callbacks.c callbacks.h \
        workspace.c workspace.h \
        guile.c guile.h \
        form.c form.h

taxbird_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

guilestuffdir = $(datadir)/taxbird/guile
guilestuff_DATA = basics-addresse.scm \
	basics-datenlieferant.scm \
	bundesland-chooser.scm \
	steuernummer.scm \
	storage.scm \
	validate.scm

autoloaddir = $(datadir)/taxbird/guile/autoload
autoload_DATA = \
	ustva-2005.scm

EXTRA_DIST = $(guilestuff_DATA) $(autoload_DATA)

BUILT_SOURCES = interface.c interface.h \
	support.c support.h
MAINTAINER_CLEAN =

support.h: support.c
support.c: interface.c

interface.h: interface.c
interface.c: ../taxbird.glade
	glade-2 -w ../taxbird.glade
