#	-*- Autoconf -*-
#
# Copyright(C) 2005 Stefan Siegl <ssiegl@gmx.de>
# taxbird - free program to interface with German IRO's Elster/Coala
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

AC_DEFUN([LIBWWW_PROGS], 
 [AC_PATH_PROG(LIBWWW_CONFIG, libwww-config)
  if test "$LIBWWW_CONFIG" = ""; then
    AC_MSG_ERROR([libwww-config not found, but required.])
  fi
  AC_SUBST(LIBWWW_CONFIG)
 ])

AC_DEFUN([LIBWWW_FLAGS],
 [AC_REQUIRE([LIBWWW_PROGS])
  
  AC_MSG_CHECKING([libwww compile flags])
  LIBWWW_CFLAGS="`$LIBWWW_CONFIG --cflags`"
  AC_MSG_RESULT([$LIBWWW_CFLAGS])

  AC_MSG_CHECKING([libwww link flags])
  LIBWWW_LDFLAGS="`$LIBWWW_CONFIG --libs`"
  AC_MSG_RESULT([$LIBWWW_LDFLAGS])

  AC_SUBST(LIBWWW_CFLAGS)
  AC_SUBST(LIBWWW_LDFLAGS)
 ])
