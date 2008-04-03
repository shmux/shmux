#! /bin/sh
#
# $Id$
#- 7
## This simple set exercises the "run" analyzer
#

ok=0

rm -rf odir
mkdir odir || exit 1
test=`../src/shmux -o odir -a run -A ./run -A 2s -e 1 -M 1 -r sh -S all -stc 'echo stdout; exit ${SHMUX_TARGET}' 1 2 3 2>&1 | grep -v second | sed 's/Analyzer for 3 timed out.*/Analyzer for 3 timed out/'`
test $? != 0 && exit 0

if [ "$test" = "    1: stdout
shmux! Child for 1 exited with status 1
    2: stdout
    2: run(2, odir)...
shmux! Analysis of 2 output indicates an error
    3: stdout
    3: run(3, odir)...
shmux! Analyzer for 3 timed out

Summary: 1 timeout, 2 errors
Timed out: 3 
Error    : 1 2 " ]; then
    ok=`expr $ok + 1`
elif [ "`uname -o 2> /dev/null`" = "Cygwin" \
       -a "$test" = "    1: stdout
shmux! Child for 1 exited with status 1
    2: stdout
    2: run(2, odir)...
shmux! Analysis of 2 output indicates an error
    3: stdout
    3: run(3, odir)...
shmux! Analysis of 3 output indicates an error

Summary: 3 errors
Error    : 1 2 3 " ]; then
    ok=`expr $ok + 1`
fi
printf "$ok/1"

test $ok = 1 && exit 77
exit 0
