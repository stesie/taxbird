#! /bin/sh
#
# Copyright(C) 2005 Stefan Siegl <stesie@brokenpipe.de>
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

LPR=lpr
MAILCAP=/etc/mailcap

TMPFILE=`mktemp` || {
  echo "$0: unable to create temporary file"
  lpr $*	# try to print anyways (data comes from stdin)
  exit 0
}

cat >> $TMPFILE # store protocol

#
# OUR FAVOURITES IN DESCENDING ORDER ...
#
if which html2text > /dev/null; then
  grep -v -e '^<?' $TMPFILE | html2text | $LPR $*
  rm -f $TMPFILE
  exit 0
fi

if which w3m > /dev/null; then
  w3m -dump -T text/html $TMPFILE | $LPR $*
  rm -f $TMPFILE
  exit 0
fi

if which lynx > /dev/null; then
  lynx -dump -force_html $TMPFILE | $LPR $*
  rm -f $TMPFILE
  exit 0
fi


#
# NOW CHECK THE MAILCAP FILE
#
if test -e $MAILCAP; then
  CMD=`grep -v -e "test=" $MAILCAP | \
    grep "^text/html; .*copiousoutput" | \
    head -1 | sed -e 's/^[^;]*;//' | sed -e 's/;.*//' | \
    sed -e "s;'\%s';$TMPFILE;"`
  ($CMD | $LPR $*) && {
    rm -f $TMPFILE
    exit 0
  }
fi

#
# CAN'T HELP, LEAVING IT TO LPR
#
$LPR $* < $TMPFILE
rm -f $TMPFILE
