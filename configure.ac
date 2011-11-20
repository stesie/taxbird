# Configure script for taxbird
# Copyright (C) 2005,2006,2007,2008,2009,2010,2011  Stefan Siegl <stesie@brokenpipe.de>
#
dnl Process this file with autoconf to produce a configure script.

#AC_INIT(configure.in)
AC_INIT(taxbird, 0.16, [taxbird@taxbird.de])
AM_INIT_AUTOMAKE(taxbird, 0.16, [taxbird@taxbird.de])
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC

# compatibility for automake < 1.8
if test -z "$mkdir_p"; then
	mkdir_p='${top_srcdir}/mkinstalldirs'
	AC_SUBST(mkdir_p)
fi

AC_HEADER_STDC

# link with export-dynamic (talking 'bout misusing libtool) ...
AC_PROG_LIBTOOL

# check for necessary gtk+ libraries
PKG_CHECK_MODULES(GTK, [gtk+-2.0])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_EXISTS([libgtkhtml-3.14],
	[PKG_CHECK_MODULES(GTKHTML, [libgtkhtml-3.14])],
	[PKG_CHECK_MODULES(GTKHTML, [libgtkhtml-3.8])])
AC_SUBST(GTKHTML_CFLAGS)
AC_SUBST(GTKHTML_LIBS)

backup_CFLAGS="$CFLAGS"
backup_LIBS="$LIBS"

#
# temporarily add libgeier to CFLAGS/LDFLAGS
# and do necessary checks
#
PKG_CHECK_MODULES(GEIER, [libgeier >= 0.7 libxml-2.0])
AC_SUBST(GEIER_CFLAGS)
AC_SUBST(GEIER_LIBS)

CFLAGS="$CFLAGS $GEIER_CFLAGS"
LIBS="$LIBS $GEIER_LIBS"

dnl Check for new-style signature functions of libgeier
AC_CHECK_FUNCS([geier_dsig_sign_softpse])


#
# now do libguile checks ...
#
GUILE_FLAGS
GUILE_MODULE_REQUIRED(ice-9 format)

CFLAGS="$backup_CFLAGS $GUILE_CFLAGS"
LIBS="$backup_LIBS $GUILE_LDFLAGS"

dnl Check for certain guile functions, introduced with libguile 1.8
AC_CHECK_FUNCS(scm_to_locale_string)
AC_CHECK_FUNCS(scm_is_string)


#
# restore old CFLAGS/LDFLAGS again
#
CFLAGS="$backup_CFLAGS"
LIBS="$backup_LIBS"


#
# gettext cruft
#
GETTEXT_PACKAGE=taxbird
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

dnl Add the languages which your application supports here.
ALL_LINGUAS="de"
AM_GLIB_GNU_GETTEXT


#
# add -wl,--export-dynamic
#
LDFLAGS="$LDFLAGS $export_dynamic_flag_spec"
AC_SUBST(wl)

#
# write Makefile
#
AC_OUTPUT([
Makefile
src/Makefile
src/guile/Makefile
src/guile/autoload/Makefile
po/Makefile.in
])

