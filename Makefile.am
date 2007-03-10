## Process this file with automake to produce Makefile.in

SUBDIRS = src po

desktopfiledir = $(prefix)/share/applications
desktopfile_DATA = taxbird.desktop

mimedatadir = $(prefix)/share/mime/packages

man_MANS = taxbird.1 taxbird-print-helper.1

EXTRA_DIST = \
	${man_MANS} \
	taxbird.xml \
	taxbird.desktop \
	taxbird.glade \
	taxbird.gladep \
	README.vendor-id \
	README.dsig

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(install_sh) -d $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	    fi \
	  done \
	fi
	$(INSTALL_DATA) $(srcdir)/pixmaps/taxbird.xpm \
		$(DESTDIR)$(datadir)/pixmaps

	test -z "$(DESTDIR)$(mimedatadir)" || $(mkdir_p) "$(DESTDIR)$(mimedatadir)"
	@list='taxbird.xml'; for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  f="`echo $$p | sed -e 's|^.*/||'`"; \
	  echo " $(INSTALL_DATA) '$$d$$p' '$(DESTDIR)$(mimedatadir)/$$f'"; \
	  $(INSTALL_DATA) "$$d$$p" "$(DESTDIR)$(mimedatadir)/$$f"; \
	done

	-update-mime-database $(prefix)/share/mime

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi
