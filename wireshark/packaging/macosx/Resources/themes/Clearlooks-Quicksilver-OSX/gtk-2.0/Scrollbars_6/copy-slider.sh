#!/bin/bash
#
# $Id: copy-slider.sh 24640 2008-03-15 18:31:52Z gerald $

cp -f slider-vert.png slider-vert-prelight.png
cp -f slider-vert.png slider-horiz-prelight.png
cp -f slider-vert.png slider-horiz.png
convert -rotate 90 slider-horiz.png slider-horiz.png
convert -rotate 90 slider-horiz-prelight.png slider-horiz-prelight.png
