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

