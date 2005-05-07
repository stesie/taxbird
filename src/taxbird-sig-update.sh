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
Usage: $0 sigfile [keyid]

Update the signature file, i.e. refresh the md5 hash value of each file
in turn and sign the sigfile afterwards using gnupg.

Taxbird expect's pathes to be relative to the guile/ subdirectory. That
is, you need to execute $0 from this directory.
EOF
    exit 0
fi

test -e $1 || {
    echo $0: specified signature file does not exist: $1
    exit 1
}

# extract names of files which shalt be signed
rm -f .sig-update.files
gpg --decrypt $1 2>/dev/null > .sig-update.files || \
    cat $1 > .sig-update.files # ... do it the brute force way ...

# generate new signature file ...
rm -f .sig-update
for FILE in `cat .sig-update.files`; do
    if [ -e $FILE ]; then
	echo "$0: adding md5 hash of file: $FILE"

	if which md5 > /dev/null; then
	    HASH=`md5 < $FILE`
	elif which md5sum > /dev/null; then
	    HASH=`md5sum < $FILE | cut -c-32`
	else
	    echo "$0: no md5 hasher available, sorry."
	    exit 1
	fi
    
	echo "$HASH  $FILE" >> .sig-update
    fi
done

rm -f .sig-update.files
if test "$2" = ""; then
    # extract key id from old signature
    unset LANG

    echo -n "$0: extracting keyid ... "
    KEYID=`gpg --verify $1 2>&1 | \
           perl -ne 'if(m/ID ([0-9A-F]{8})/){print "$1\n";}'`

    if [ "$KEYID" = "" ]; then
	echo "failed."
	exit 1
    fi

    echo $KEYID
else
    KEYID=$2;
fi

(gpg --list-secret-keys 2>/dev/null | grep -ie $KEYID > /dev/null) || {
    echo "$0: secret key not available: $KEYID"
    echo "Sorry."
    exit 1
}

(gpg --decrypt $1 2>/dev/null | diff .sig-update - > /dev/null) && {
    echo "$0: signature file unchanged. stopping here."
    exit 0
}

set -e

if grep -q -e "-----BEGIN PGP SIGNED MESSAGE-----" < $1; then
	perl -pe 'm/-----BEGIN PGP SIGNED MESSAGE-----/ and exit;' < $1 \
		> .sig-update.signed
else
	echo '$Id: taxbird-sig-update.sh,v 1.3 2005-05-07 16:34:52 stesie Exp $' > .sig-update.signed
fi

gpg --armor --clearsign --local-user $KEYID < .sig-update >> .sig-update.signed
rm -f .sig-update
mv -f .sig-update.signed $1
    
