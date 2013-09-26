#!/bin/bash
export CFLAGS="-g -O2"


export CONFIGURE_FLAGS="--prefix=/usr --sysconfdir=/usr/share --datadir=/usr/share --libdir=/usr/lib/wireshark --disable-static --enable-warnings-as-errors=no --enable-setuid-install --with-plugins=/usr/lib/wireshark/plugins --with-lua=/usr/"

cp /usr/share/misc/config.guess /usr/share/misc/config.sub .
libtoolize --force --copy
mkdir aclocal-missing
./autogen.sh
CFLAGS="$(CFLAGS)" ./configure $(CONFIGURE_FLAGS)

