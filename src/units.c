/*
** Copyright (C) 2002 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#include <ctype.h>

#include "units.h"

static char const rcsid[] = "@(#)$Id: units.c,v 1.1 2002-07-07 03:54:18 kalt Exp $";

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
	  exit(1);
      default:
	  fprintf(stderr, "%s: Invalid time unit: %c\n", myname, *unit);
	  exit(1);
      }
}
