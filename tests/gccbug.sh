#! /bin/sh
#
# $Id: gccbug.sh,v 1.1 2004-04-06 00:36:55 kalt Exp $
#- 1
## This simple test should be no problem, but detects an odd gcc bug seen
## with gcc 3.0.4 on Solaris 8 when shmux is compiled with -Ox (x>=2).
## Removing the optimization flag in src/Makefile should fix it.
## The code has changed significantly since this was found out, so whether
## or not this problem would still be present is unclear.
#

test=`../src/shmux -bsQtc 'echo ${SHMUX_TARGET}' sh:one 2>&1`;

if [ "x${test}" = "xone" ]; then
    printf "1/1"
    exit 77
else
    printf "0/1"
    exit 0
fi

