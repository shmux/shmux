#! /bin/sh
#
# $Id$
#- 3
## This set of tests validates -e/-E usage with or without -q/-qq
#

ok=0

test=`../src/shmux -e 10-,-2,4,6-8 -E 1-5 -M 1 -r sh -S all -bstc 'exit ${SHMUX_TARGET}' 1 2 3 4 5 6 7 8 9 10 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = "shmux! Child for 1 exited with status 1
shmux! Child for 2 exited with status 2
shmux: Child for 3 exited with status 3
shmux! Child for 4 exited with status 4
shmux: Child for 5 exited with status 5
shmux! Child for 6 exited with status 6
shmux! Child for 7 exited with status 7
shmux! Child for 8 exited with status 8
shmux! Child for 10 exited with status 10

Summary: 3 successes, 7 errors
Error    : 1 2 4 6 7 8 10 " ]; then
    ok=`expr $ok + 1`
fi
printf "$ok/1"

rm -rf odir
mkdir odir || exit 1
test=`../src/shmux -o odir -q -e 10-,-2,4,6-8 -E 1-5 -M 1 -r sh -S all -stc 'echo ${SHMUX_TARGET}; exit ${SHMUX_TARGET}' 1 2 3 4 5 6 7 8 9 10 2>&1`
test $? != 0 && exit 0
test=`echo "$test" | grep -v second`

if [ "$test" = "    1: 1
shmux! Child for 1 exited with status 1
    2: 2
shmux! Child for 2 exited with status 2
shmux: Child for 3 exited with status 3
    4: 4
shmux! Child for 4 exited with status 4
shmux: Child for 5 exited with status 5
    6: 6
shmux! Child for 6 exited with status 6
    7: 7
shmux! Child for 7 exited with status 7
    8: 8
shmux! Child for 8 exited with status 8
   10: 10
shmux! Child for 10 exited with status 10

Summary: 3 successes, 7 errors
Error    : 1 2 4 6 7 8 10 " ]; then
    ok=`expr $ok + 1`
fi
test -d exitcode/.svn && mv exitcode/.svn .oink
diff -r exitcode odir > /dev/null
if [ $? != 0 ]; then
    test -d .oink && mv .oink exitcode/.svn
    exit 0
fi
printf "\b\b\b$ok/2"
test -d .oink && mv .oink exitcode/.svn

rm -rf odir
mkdir odir || exit 1
test=`../src/shmux -o odir -qq -e 10-,-2,4,6-8 -E 1-5 -M 1 -r sh -S all -stc 'echo ${SHMUX_TARGET}; exit ${SHMUX_TARGET}' 1 2 3 4 5 6 7 8 9 10 2>&1`
test $? != 0 && exit 0
test=`echo "$test" | grep -v second`

if [ "$test" = "shmux! Child for 1 exited with status 1
shmux! Child for 2 exited with status 2
shmux: Child for 3 exited with status 3
shmux! Child for 4 exited with status 4
shmux: Child for 5 exited with status 5
shmux! Child for 6 exited with status 6
shmux! Child for 7 exited with status 7
shmux! Child for 8 exited with status 8
shmux! Child for 10 exited with status 10

Summary: 3 successes, 7 errors
Error    : 1 2 4 6 7 8 10 " ]; then
    test -d exitcode/.svn && mv exitcode/.svn .oink
    diff -r exitcode odir > /dev/null
    if [ $? = 0 ]; then
        ok=`expr $ok + 1`
    fi
fi
printf "\b\b\b$ok/3"
test -d .oink && mv .oink exitcode/.svn

test $ok = 3 && exit 77
exit 0
