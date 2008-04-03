/*
** Copyright (C) 2002, 2003, 2004, 2005, 2006 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#include <ctype.h>

#include "units.h"

static char const rcsid[] = "@(#)$Id$";

extern char *myname;

u_int
unit_time(timestr)
char *timestr;
{ 
    char *unit;

    unit = timestr;
    while (*unit != '\0' && isdigit((int) *unit) != 0)
	unit += 1;
    switch (*unit)
      {
      case 'W':
      case 'w':
	  return 7*24*60*60 * atoi(timestr);
      case 'D':
      case 'd':
	  return 24*60*60 * atoi(timestr);
      case 'H':
      case 'h':
	  return 60*60 * atoi(timestr);
      case 'M':
      case 'm':
	  return 60 * atoi(timestr);
      case 'S':
      case 's':
	  return atoi(timestr);
      case '\0':
	  fprintf(stderr, "%s: No unit specified for '%s'\n", myname, timestr);
	  exit(RC_ERROR);
      default:
	  fprintf(stderr, "%s: Invalid time unit: %c\n", myname, *unit);
	  exit(RC_ERROR);
      }
}

char *
unit_rtime(u_int timeval)
{
    static char timestr[80];
    int width;

    if (timeval == 0)
	return "0s";

    timestr[0] = '\0';
    if (timeval > 7*24*60*60)
      {
	sprintf(timestr + strlen(timestr), "%uw",
		(u_int) (timeval / (7*24*60*60)));
        width = 2;
      }
    else
        width = 1;
    timeval %= 7*24*60*60;
    if (timeval > 24*60*60)
      {
	sprintf(timestr + strlen(timestr), "%ud",
                (u_int) (timeval / (24*60*60)));
        width = 2;
      }
    timeval %= 24*60*60;
    if (timeval > 60*60)
      {
	sprintf(timestr + strlen(timestr), "%.*uh",
                width, (u_int) (timeval / (60*60)));
        width = 2;
      }
    timeval %= 60*60;
    if (timeval > 60)
      {
	sprintf(timestr + strlen(timestr), "%.*um",
                width, (u_int) (timeval / 60));
        width = 2;
      }
    timeval %= 60;
    if (timeval > 0)
	sprintf(timestr + strlen(timestr), "%.*us", width, timeval);

    return timestr;
}
