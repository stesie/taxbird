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
        form.c form.h \
	digest.c

taxbird_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

guilestuffdir = $(datadir)/taxbird/guile
guilestuff_DATA = basics-addresse.scm \
	basics-datenlieferant.scm \
	bundesland-chooser.scm \
	zeitraum-chooser.scm \
	steuernummer.scm \
	storage.scm \
	validate.scm \
	xml-writer.scm \
	export.scm \
	ustva-2005.scm

autoloaddir = $(datadir)/taxbird/guile/autoload
autoload_DATA = \
	sigtest.scm \
	ustva-2005-stub.scm

signaturesdir = $(datadir)/taxbird/guile/signatures
signatures_DATA = \
	sigtest.sig

gnupgkeysdir = $(datadir)/taxbird/pubkeys
gnupgkeys_DATA = \
	stesie.pub

EXTRA_DIST = \
	taxbird-sig-update.sh \
	$(guilestuff_DATA) \
	$(autoload_DATA) \
	$(signatures_DATA) \
	$(gnupgkeys_DATA) \
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

