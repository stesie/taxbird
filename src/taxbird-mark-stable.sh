#! /bin/sh
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

CVS=/usr/bin/cvs
STABLE=openssl_stable

if test "$1" = "--help" -o "$1" = "-h"; then
    cat <<EOF
Usage: $0

Mark current files in guile/ subdirectory as 'stable', i.e.
make taxbird-update.sh update to this version.
EOF
    exit 0
fi

test -d "guile" && cd guile
test -e "validate.scm" || {
    cat <<__FOO
Directory `pwd` does not contain validate.scm file,
it does not seem to be the taxbird guile/ directory.

Sorry.
__FOO
    exit 1
}

test -d "CVS" || {
    cat <<__FOO
There is no CVS/ subdirectory below guile/. It seems
like you don't have a CVS checkout available, however 
that is necessary. Sorry.
__FOO
    exit 1
}

$CVS -z5 tag -cFR $STABLE || {
    echo "CVS call failed, sorry."
    exit 1
}

echo "succeeded."
