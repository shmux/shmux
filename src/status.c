/*
** Copyright (C) 2002 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"
#include <time.h>
#if defined(HAVE_SYS_LOADAVG_H)
# include <sys/loadavg.h>
#endif

#include "status.h"

#include "target.h"
#include "term.h"

static char const rcsid[] = "@(#)$Id: status.c,v 1.3 2002-07-06 18:08:00 kalt Exp $";

static int spawned, inphase[4];
static time_t changed[4];

/*
** status_init
**	Initialize things..
*/
void
status_init(pings, tests)
int pings, tests;
{
    spawned = inphase[0] = inphase[1] = inphase[2] = inphase[3] = 0;
    changed[0] = changed[1] = changed[2] = changed[3] = 0;
    if (pings == 0)
	inphase[1] = -1;
    if (tests == 0)
	inphase[2] = -1;
}

/*
** status_spawned
**	Update the number of children rowming free.
*/
void
status_spawned(count)
int count;
{
    spawned += count;
}

/*
** status_phase
**	Update the number of target in a particular phase.
*/
void
status_phase(phase, count)
int phase, count;
{
    assert( phase >= -1 && phase < 4 );
    assert( inphase[0] != -1 );

    while (inphase[phase] == -1)
	phase -= 1;
    if (phase == 0)
	return;
    if (phase == -1)
	phase = 0;
    inphase[phase] += count;
    changed[phase] = time(NULL);
}

/*
** status_update
**	Call sprint
*/
void
status_update(void)
{
    time_t now;
    char loadavg[20];
#if defined(HAVE_GETLOADAVG) && !defined(GETLOADAVG_PRIVILEGED)
    double load[3];
    static erroronce = 1;

    loadavg[0] = '\0';
    if (getloadavg(load, 3) == -1)
      {
	if (erroronce == 1)
	  {
	    eprint("getloadavg(): %s", strerror(errno));
	    erroronce = 0;
	  }
      }
    else
	sprintf(loadavg, "[%.1f, %.1f]", load[0], load[1]);
#else
    loadavg[0] = '\0';
#endif
	
    now = time(NULL);
    if (inphase[1] == -1 && inphase[2] == -1)
	sprint("-- %d Active, %d Pending/%s%d Failed/%s%d Done -- %s",
	       spawned, target_getmax() - inphase[0] - inphase[3],
	       (now - changed[0] < 2) ? "\a" : "", inphase[0],
	       (now - changed[3] < 2) ? "\a" : "", inphase[3],
	       loadavg);
    else if (inphase[2] == -1)
	sprint("-- %d Active, %d Pending/%s%d Failed/%s%d Alive/%s%d Done -- %s",
	       spawned, target_getmax() - inphase[0] - inphase[1] - inphase[3],
	       (now - changed[0] < 2) ? "\a" : "", inphase[0],
	       (now - changed[1] < 2) ? "\a" : "", inphase[1],
	       (now - changed[3] < 2) ? "\a" : "", inphase[3],
	       loadavg);
    else if (inphase[1] == -1)
	sprint("-- %d Active, %d Pending/%s%d Failed/%s%d OK/%s%d Done -- %s",
	       spawned, target_getmax() - inphase[0] - inphase[2] - inphase[3],
	       (now - changed[0] < 2) ? "\a" : "", inphase[0],
	       (now - changed[2] < 2) ? "\a" : "", inphase[2],
	       (now - changed[3] < 2) ? "\a" : "", inphase[3],
	       loadavg);
    else
	sprint("-- %d Active, %d Pending/%s%d Failed/%s%d Alive/%s%d OK/%s%d Done -- %s",
	       spawned, target_getmax() - inphase[0] - inphase[1] - inphase[2]
	       - inphase[3],
	       (now - changed[0] < 2) ? "\a" : "", inphase[0],
	       (now - changed[1] < 2) ? "\a" : "", inphase[1],
	       (now - changed[2] < 2) ? "\a" : "", inphase[2],
	       (now - changed[3] < 2) ? "\a" : "", inphase[3],
	       loadavg);
}
