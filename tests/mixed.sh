#! /bin/sh
#
# $Id: mixed.sh,v 1.1 2004-04-06 00:36:55 kalt Exp $
#- 2
## Simple test for -m
#

ok=0

test=`../src/shmux -r sh -S all -bsQtc 'sleep ${SHMUX_TARGET}; echo ${SHMUX_TARGET}; sleep ${SHMUX_TARGET}; echo ${SHMUX_TARGET}' 1 3 4 2>&1`

if [ "$test" = "1
1
3
4
3
4" ]; then
    ok=`expr $ok + 1`
fi
printf "$ok/1"

test=`../src/shmux -m -r sh -S all -bsQtc 'sleep ${SHMUX_TARGET}; echo ${SHMUX_TARGET}; sleep ${SHMUX_TARGET}; echo ${SHMUX_TARGET}' 1 3 4 2>&1`

if [ "$test" = "1
1
3
3
4
4" ]; then
    ok=`expr $ok + 1`
fi
printf "\b\b\b$ok/2"

test $ok = 2 && exit 77
exit 0
