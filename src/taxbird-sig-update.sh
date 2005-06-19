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

if test "$1" = "--help" -o "$1" = "-h"; then
    cat <<EOF
Usage: $0 sigfile cert-file privkey-file

Update the signature file, i.e. refresh the md5 hash value of each file
in turn and sign the sigfile afterwards using openssl s/mime.

Taxbird expect's pathes to be relative to the guile/ subdirectory. That
is, you need to execute $0 from this directory.
EOF
    exit 0
fi

(test "x$1" != "x" && test -e "$1") || {
    echo $0: specified signature file does not exist: $1
    exit 1
}

(test "x$2" != "x" && test -e "$2") || {
    echo $0: specified certificate file does not exist: $2
    exit 1
}

(test "x$3" != "x" && test -e "$3") || {
    echo $0: specified private key file does not exist: $3
    exit 1
}

# extract names of files which shalt be signed
rm -f .sig-update.files
openssl smime -noverify -verify -in $1 > .sig-update.files || \
    cat $1 > .sig-update.files # ... do it the brute force way ...

# generate new signature file ...
rm -f .sig-update
for FILE in `tr -d "\r" < .sig-update.files`; do
    if [ -e $FILE ]; then
	echo "$0: adding md5 hash of file: $FILE"

	HASH=`openssl md5 < $FILE`
	echo "$HASH  $FILE" >> .sig-update
    fi
done

rm -f .sig-update.files

set -e
openssl smime -sign -signer $2 -inkey $3 -in .sig-update \
    -out .sig-update.signed
rm -f .sig-update

cat $2 > $1
echo "Taxbird-Id: \$Id: taxbird-sig-update.sh,v 1.4 2005-06-19 15:22:14 stesie Exp $1,v 0.9 (not checked in yet) \$" >> $1
tail -n +1 .sig-update.signed >> $1

rm -f .sig-update.signed
    
