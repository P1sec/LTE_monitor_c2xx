#!/bin/sh
#
# $Id: getdisplay.sh 24640 2008-03-15 18:31:52Z gerald $
#
# Author: Aaron Voisine <aaron@voisine.org>

if [ "$DISPLAY"x == "x" ]; then
    echo :0 > /tmp/display.$UID
else
    echo $DISPLAY > /tmp/display.$UID
fi
