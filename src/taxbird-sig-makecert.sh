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
Usage: $0 keyfile [days]

Generate a new private rsa key as well as an X.509 certificate, 
needed in order to create your own taxbird signature files.

Days is the number of days, the certificate shall be valid, i.e.
how long the taxbird user's shall be able to use the signed 
version of code.  Defaults to 90 days.
EOF
    exit 0
fi

test "x$1" = "x" && {
    echo "See $0 --help for help on how to call this script"
    exit 1
}

set -e
openssl genrsa -des3 1024 > $1.pem

DAYS=90
test "x$2" = "x" || DAYS=$2

cat <<EOF
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++                                                                    +++
+++  Now we will create a X.509 certificate for your private (vendor)  +++
+++  key.  Just fill out the information openssl asks for.  If you're  +++
+++  asked for the 'Organizational Unit Name', make sure to enter the  +++
+++  vendor id, you got from the German inland revenue office!         +++
+++                                                                    +++
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
EOF

set -x

openssl req -new -nodes -sha1 -key $1.pem -out $1.req
openssl x509 -req -in $1.req -signkey $1.pem -days $DAYS -sha1 -out $1.crt
rm -f $1.req

set +x
echo
echo "OpenSSL certificates successfully created."
echo "That's what you have now: "
echo
echo "X.509 Certificate (public key part):   $1.crt"
echo "1024 bit RSA Private Key:              $1.pem"
echo
echo "Please mind, that $1.pem file is to be considered private!"
echo