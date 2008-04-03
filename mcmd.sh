#! /bin/sh
#
# Sample wrapper script using shmux(1) to execute another command "CMD"
# on multiple hosts.
#
# The following script cannot be used as-is, it is only provided as a sample
# and starting point.  Look for XXX to see where changes are required.
#
# $Id$
#

# XXX CMDOPTS should be replaced with any option one may want to pass to
# XXX the command being wrapped.
USAGE="Usage: `basename $0` [ -h | [ -q ] [ -M <max> ] [ -T <timeout> ] [ -CMDOPTS ] ] target1 target2 ..."

set --  `getopt hqMTCMDOPTS $*`
if [ $? != 0 ]
then
    echo $USAGE 1>&2
    exit 1
fi

CMD_FLAGS=""
SHMUX_FLAGS=""
quiet=0

for i in $*
do
    case "$i" in
        -h)		echo $USAGE; exit 0;;
	-CMDOPTS)
			CMD_FLAGS="$CMD_FLAGS $i"; shift;;
	-M)		SHMUX_FLAGS="$SHMUX_FLAGS $1 $2"; shift 2;;
	-T)		SHMUX_FLAGS="$SHMUX_FLAGS -C $2"; shift 2;;
	-q)		quiet=1;;
	--)		shift; break;;
    esac
done

abort=0
catch () {
    abort=1
}
stage="started"
cleanup () {
    if [ $abort = 0 -a $stage != "done" ]
    then
	# Useful for after the fact debugging/inspection
	echo "Something went wrong, /tmp/mux.$$ will NOT be removed."
	if [ $stage != "report" ]
	then
	    echo "shmux said:"
	    echo
	    echo "$MUX"
	fi
    else
	rm -rf /tmp/mux.$$
    fi
}

umask 022

test -d /tmp/mux.$$ \
&& ( echo "`basename $0`: /tmp/mux.$$ already exists!" 1>&2 ; exit 1 )
mkdir /tmp/mux.$$ || exit 1

trap "catch" 2
trap "cleanup" 0 1 3 15

# XXX CMD needs to be replaced with the real command name, obviously
# XXX This is also where you may want to customize the way shmux is invoked
MUX=`shmux -qq -o /tmp/mux.$$ -p -r sh $SHMUX_FLAGS -c "CMD $CMD_FLAGS \\$SHMUX_TARGET" $* 2>&1`

if [ $abort != "0" ]
then
    echo "`basename $0`: Aborted!"
    exit 1
fi

muxwc="`echo \"$MUX\" | wc -l`"

# Minimal preprocessing of ost results
out=0
minost=86400
minhost=""
maxost=0
maxhost=""
for i in /tmp/mux.$$/*.stdout
do
    if [ -s "$i" ]
    then
	# First get the stripped report.
	egrep -v '^##' $i > $i.stripped
	if [ -s "$i.stripped" ]
	then
	    out=`expr $out + 1`
	else
	    rm -f "$i.stripped"
	fi
	# Second get the processing time for stats later.
	time=`awk '/^## Host .* processed in [0-9]+/ { print $6 }' < $i`
	if [ -n "$time" ]
	then
	    if [ $time -gt $maxost ]
		then
		maxost=$time
		maxhost=`basename $i | sed 's/.stdout$//'`
	    fi
	    if [ $time -lt $minost ]
	    then
		minost=$time
		minhost=`basename $i | sed 's/.stdout$//'`
	    fi
	fi
    else
	# May be worth issuing a warning here, this shouldn't happen.
	# (But if it does, there should be some error message somewhere..)
	rm -f "$i"
    fi
done

# Count number of host with ost errors
err=0
for i in /tmp/mux.$$/*.stderr
do
    if [ -s "$i" ]
    then
	err=`expr $err + 1`
    else
	rm -f "$i"
    fi
done

# If everything is cool and we got to be quiet, then be quiet!
if [ "$muxwc" = 2 -a $out -eq 0 -o $err -eq 0 -a $quiet = 1 ]
then
    stage="done"
    exit 0
fi

# Quick overall stats
echo "$MUX"
echo "Fastest host: $minhost ($minost seconds)"
echo "Slowest host: $maxhost ($maxost seconds)"
echo "$out host(s) with divergences, $err host(s) with errors"
echo "==============================================================================="
echo

stage="report"

# First report errors
if [ $err -gt 0 ]
then
    for i in /tmp/mux.$$/*.stderr
    do
        echo "=== Errors for `basename $i | sed 's/.stderr$//'`"
	cat $i
	rm -f $i
    done
    echo
fi

# Second consolidate results
if [ $out -gt 0 ]
then
    for i in /tmp/mux.$$/*.stdout.stripped
    do
        test -f $i || continue
	echo "==============================================================================="
	echo "=== `basename $i | sed 's/.stdout.stripped$//'`"
	for j in /tmp/mux.$$/*.stdout.stripped
	do
	    cmp -s $i $j
	    if [ "$i" != "$j" -a $? -eq 0 ]
	    then
		echo "=== `basename $j | sed 's/.stdout.stripped$//'`"
		rm -f $j
	    fi
	done
	echo "==============================================================================="
	cat $i
	rm -f $i
	echo
    done
fi

# Finally show timing details
egrep '^## Host .* processed in [0-9]+' /tmp/mux.$$/*.stdout | sed 's/.*:##/##/'

stage="done"
exit 0
