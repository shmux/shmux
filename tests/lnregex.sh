#! /bin/sh
#
# $Id$
#- 5
## This set of tests exercises the "lnregex" analyzer
#

ok=0

rm -rf odir
mkdir odir || exit 1
test=`../src/shmux -o odir -a lnre -M 1 -r sh -S all -stc 'echo stdout; echo unknown 1>&${SHMUX_TARGET}; exit 0' 1 2 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = "    1: stdout
    1: unknown
    2: stdout
shmux! Analysis of 2 output indicates an error
    2! unknown

Summary: 1 success, 1 error
Error    : 2 " ]; then
    ok=`expr $ok + 1`
fi
printf "$ok/1"

rm -rf odir
mkdir odir || exit 1
test=`../src/shmux -o odir -a lnre -A lnregex.out -A lnregex.err -M 1 -r sh -S all -stc 'echo notices are fine; echo anything with okay in there is fine too; echo unless it is notokay; echo random stuff is bad too; echo stderr warnings are okay 1>&2; echo other stderr stuff is bad 1>&2; exit 0' oink 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = " oink: notices are fine
 oink: anything with okay in there is fine too
shmux! Analysis of oink output indicates an error
 oink: unless it is notokay
 oink: random stuff is bad too
 oink! stderr warnings are okay
 oink! other stderr stuff is bad

Summary: 1 error
Error    : oink " ]; then
    ok=`expr $ok + 1`
fi
printf "\b\b\b$ok/2"

rm -rf odir
mkdir odir || exit 1
test=`../src/shmux -o odir -a lnre -A lnregex.out -A lnregex.err -M 1 -r sh -S all -stc 'echo notices are fine; echo anything with okay in there is fine too; echo random stuff is bad; echo random stderr stuff is bad 1>&2; exit 0' oink 2>&1 | grep -v second`
test $? != 0 && exit 0

if [ "$test" = " oink: notices are fine
 oink: anything with okay in there is fine too
shmux! Analysis of oink output indicates an error
 oink: random stuff is bad
 oink! random stderr stuff is bad

Summary: 1 error
Error    : oink " ]; then
    ok=`expr $ok + 1`
fi
printf "\b\b\b$ok/3"

test $ok = 3 && exit 77
exit 0
