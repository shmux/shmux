#! /bin/sh
#
# $Id: regex.sh,v 1.1 2004-04-06 00:36:55 kalt Exp $
#- 6
## This set of tests exercises the "regex" analyzer
#

ok=0

rm -rf odir
mkdir odir || exit 1
# Basic one liner check, default stderr
test=`../src/shmux -o odir -a regex -A . -M 1 -r sh -S all -stc 'echo stdout; echo unknown 1>&${SHMUX_TARGET}; exit 0' 1 2 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = "    1: stdout
    1: unknown
    2: stdout
    2! unknown
shmux! Analysis of 2 output indicates an error

Summary: 1 success, 1 error
Error    : 2 " ]; then
    ok=`expr $ok + 1`
fi
printf "$ok/1"

rm -rf odir
mkdir odir || exit 1
# Same as above, different format
test=`../src/shmux -o odir -a regex -A =. -M 1 -r sh -S all -stc 'echo stdout; echo unknown 1>&${SHMUX_TARGET}; exit 0' 1 2 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = "    1: stdout
    1: unknown
    2: stdout
    2! unknown
shmux! Analysis of 2 output indicates an error

Summary: 1 success, 1 error
Error    : 2 " ]; then
    ok=`expr $ok + 1`
fi
printf "\b\b\b$ok/2"

rm -rf odir
mkdir odir || exit 1
# Still basic, negative condition
test=`../src/shmux -o odir -a regex -A !a -M 1 -r sh -S all -stc 'echo stdout; echo unknown 1>&${SHMUX_TARGET}; exit 0' 1 2 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = "    1: stdout
    1: unknown
    2: stdout
    2! unknown
shmux! Analysis of 2 output indicates an error

Summary: 1 success, 1 error
Error    : 2 " ]; then
    ok=`expr $ok + 1`
fi
printf "\b\b\b$ok/3"

rm -rf odir
mkdir odir || exit 1
# Multi line
test=`../src/shmux -o odir -a regex -A '^stdout
unknown$' -M 1 -r sh -S all -stc 'echo stdout; echo unknown; exit 0' 1 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = "    1: stdout
    1: unknown

Summary: 1 success" ]; then
    ok=`expr $ok + 1`
fi
printf "\b\b\b$ok/4"

rm -rf odir
mkdir odir || exit 1
# Another multi line
test=`../src/shmux -o odir -a regex -A '^stdout[\nunknown]*$' -A '.*' -M 1 -r sh -S all -stc 'echo stdout; echo unknown 1>&${SHMUX_TARGET}; exit 0' 1 2 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = "    1: stdout
    1: unknown
    2: stdout
    2! unknown

Summary: 2 successes" ]; then
    ok=`expr $ok + 1`
fi
printf "\b\b\b$ok/5"

rm -rf odir
mkdir odir || exit 1
# Regex in files
test=`../src/shmux -o odir -a regex -A '<regex.out' -A '=<regex.err' -M 1 -r sh -S all -stc 'echo stdout; echo unknown 1>&${SHMUX_TARGET}; exit 0' 1 2 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = "    1: stdout
    1: unknown
    2: stdout
    2! unknown

Summary: 2 successes" ]; then
    ok=`expr $ok + 1`
fi
printf "\b\b\b$ok/6"

test $ok = 6 && exit 77
exit 0
