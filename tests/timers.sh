#! /bin/sh
#
# $Id$
#- 4
## This simple set exercises the implementation of timeouts
#

ok=0

rm -rf odir
mkdir odir || exit 1
test=`../src/shmux -C 2s -M 1 -v -r sh -S all -stc 'echo "Slumber.. mmm!"
if [ $SHMUX_TARGET = 1 ]; then
    echo "Sleeping 7 seconds.."
    sleep 7
elif [ $SHMUX_TARGET = 2 ]; then
    trap "" 14
    trap "exit 0" 15
    echo "Sleeping 10 seconds.."
    sleep 10
elif [ $SHMUX_TARGET = 3 ]; then
    trap "" 14 15
    echo "Sleeping 15 seconds.."
    sleep 15
fi

exit 0' 0 1 2 3 2>&1 | grep -v " targets processed in " | grep -v "with status " | grep -v "2! " | sed 's/timed out.*/timed out/'`
test $? != 0 && exit 0

if [ "$test" = "    0: Slumber.. mmm!
    1: Slumber.. mmm!
    1: Sleeping 7 seconds..
shmux! Child for 1 timed out
    2: Slumber.. mmm!
    2: Sleeping 10 seconds..
shmux: Time out for 2 (Sending SIGTERM)..
shmux! Child for 2 timed out
    3: Slumber.. mmm!
    3: Sleeping 15 seconds..
shmux: Time out for 3 (Sending SIGTERM)..
shmux: Time out for 3 (Sending SIGKILL)..
shmux! Child for 3 timed out

Summary: 3 timeouts, 1 success
Timed out: 1 2 3 " ]; then
    ok=`expr $ok + 1`
elif [ "$test" = "    0: Slumber.. mmm!
    1: Slumber.. mmm!
    1: Sleeping 7 seconds..
shmux! Child for 1 timed out
    2: Slumber.. mmm!
    2: Sleeping 10 seconds..
shmux: Time out for 2 (Sending SIGTERM)..
    3: Slumber.. mmm!
    3: Sleeping 15 seconds..
shmux: Time out for 3 (Sending SIGTERM)..
shmux: Time out for 3 (Sending SIGKILL)..
shmux! Child for 3 timed out

Summary: 2 timeouts, 2 successes
Timed out: 1 3 " ]; then
    # This is an aberration, but it's a known one.
    ok=`expr $ok + 1`
elif [ "`uname -o 2> /dev/null`" = "Cygwin" \
       -a "$test" = "    0: Slumber.. mmm!
    1: Slumber.. mmm!
    1: Sleeping 7 seconds..
    2: Slumber.. mmm!
    2: Sleeping 10 seconds..
shmux: Time out for 2 (Sending SIGTERM)..
    3: Slumber.. mmm!
    3: Sleeping 15 seconds..
shmux: Time out for 3 (Sending SIGTERM)..
shmux: Time out for 3 (Sending SIGKILL)..
shmux! Child for 3 timed out

Summary: 1 timeout, 3 successes
Timed out: 3 " ]; then
    ok=`expr $ok + 1`
fi
printf "$ok/1"

test $ok = 1 && exit 77
exit 0
