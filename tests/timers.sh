#! /bin/sh
#
# $Id: timers.sh,v 1.1 2004-04-06 00:36:55 kalt Exp $
#- 4
## This simple set exercises the implementation of timeouts
#

ok=0

rm -rf odir
mkdir odir || exit 1
test=`../src/shmux -C 2s -M 1 -r sh -S all -stc 'echo "Slumber.. mmm!"
if [ $SHMUX_TARGET = 1 ]; then
    echo "Sleeping 5 seconds.."
    sleep 7
elif [ $SHMUX_TARGET = 2 ]; then
    trap "" ALRM
    echo "Sleeping 10 seconds.."
    sleep 10
elif [ $SHMUX_TARGET = 3 ]; then
    trap "" ALRM TERM
    echo "Sleeping 15 seconds.."
    sleep 15
fi

exit 0' 0 1 2 3 2>&1 | grep -v second | sed 's/timed out.*/timed out/`
test $? != 0 && exit 0

if [ "$test" = "    0: Slumber.. mmm!
    1: Slumber.. mmm!
shmux! Child for 1 timed out
    2: Slumber.. mmm!
shmux! Child for 2 timed out
    3: Slumber.. mmm!
shmux! Child for 3 timed out

Summary: 3 timeouts, 1 success
Timed out: 1 2 3 " ]; then
    ok=`expr $ok + 1`
fi
printf "$ok/1"

test $ok = 1 && exit 77
exit 0
