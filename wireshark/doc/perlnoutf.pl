#!/usr/bin/perl -w

# $Id: perlnoutf.pl 28784 2009-06-21 12:47:48Z morriss $

# Call another Perl script, passing our caller's arguments, with
# environment variables unset so perl doesn't interpret bytes as UTF-8
# characters.

use strict;

delete $ENV{LANG};
delete $ENV{LANGUAGE};
delete $ENV{LC_ALL};
delete $ENV{LC_CTYPE};

system("$^X -w @ARGV");
