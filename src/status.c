/*
** Copyright (C) 2002-2008 Christophe Kalt
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

#if defined(MAX)
# undef MAX
#endif
#define MAX(a, b) ((a > b) ? a : b)

static char const rcsid[] = "@(#)$Id$";

static int spawned, inphase[5];
static time_t spawnedchg, changed[5];

/*
** status_init
**	Initialize things..
*/
void
status_init(pings, tests, analyzer)
int pings, tests, analyzer;
{
    spawned = 0;
    inphase[0] = inphase[1] = inphase[2] = inphase[3] = inphase[4] = 0;
    spawnedchg = changed[0] = changed[1] = changed[2] = changed[3] = 0;
    if (pings == 0)
	inphase[1] = -1;
    if (tests == 0)
	inphase[2] = -1;
    if (analyzer == 0)
	inphase[4] = -1;
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
    spawnedchg = time(NULL);
}

/*
** status_phase
**	Update the number of target in a particular phase.
*/
void
status_phase(phase, count)
int phase, count;
{
    assert( phase >= -1 && phase < 5 );
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
    char tmp[4][80], loadavg[20], active[16];
#if defined(HAVE_GETLOADAVG) && !defined(GETLOADAVG_PRIVILEGED)
    double load[3];
    static int erroronce = 1;

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
	snprintf(loadavg, sizeof(loadavg), "[%.1f, %.1f]", load[0], load[1]);
#else
    loadavg[0] = '\0';
#endif

    now = time(NULL);

    if (spawned == 0 && spawnedchg > 0 && (now - spawnedchg) > 1)
	strlcpy(active, "\a[PAUSED]\a", sizeof(active));
    else
	snprintf(active, sizeof(active), "%d Active", spawned);
    if (inphase[1] >= 0)
	snprintf(tmp[0], sizeof(tmp[0]), "%s%d Alive/",
		(now - changed[1] < 2) ? "\a" : "", inphase[1]);
    else
	tmp[0][0] = '\0';

    if (inphase[2] >= 0)
	snprintf(tmp[1], sizeof(tmp[1]), "%s%d OK/",
		(now - changed[2] < 2) ? "\a" : "", inphase[2]);
    else
	tmp[1][0] = '\0';

    if (inphase[4] >= 0)
      {
	snprintf(tmp[2], sizeof(tmp[2]), "%s%d Run/",
		(now - changed[3] < 2) ? "\a" : "", inphase[3]);
	snprintf(tmp[3], sizeof(tmp[3]), "%s%d Done",
		(now - changed[4] < 2) ? "\a" : "", inphase[4]);
      }
    else
      {
	snprintf(tmp[2], sizeof(tmp[2]), "%s%d Done",
		(now - changed[3] < 2) ? "\a" : "", inphase[3]);
	tmp[3][0] = '\0';
      }

    sprint("-- %s, %d Pending/%s%d Failed/%s%s%s%s -- %s",
	   active,
	   target_getmax() - inphase[0] - MAX(0, inphase[1])
	   - MAX(0, inphase[2]) - inphase[3] - MAX(0, inphase[4]),
	   (now - changed[0] < 2) ? "\a" : "", inphase[0],
	   tmp[0], tmp[1], tmp[2], tmp[3], loadavg);
}
