/*
** Copyright (C) 2003 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#if defined(HAVE_DECL_SYS_SIGNAME)
# include <signal.h>
#endif

#include "siglist.h"
#include "signals.h"

static char const rcsid[] = "@(#)$Id$";

int
getsignumbyname(name)
char *name;
{
    int i;

    i = 0;
#if defined(HAVE_SYS_SIGNAME)
    while (i < NSIG)
      {
	if (strcmp(sys_signame[i], name) == 0)
	    return i;
#else
    while (signame[i].name != NULL)
      {
	if (strcmp(signame[i].name, name) == 0)
	    return signame[i].num;
#endif
	i += 1;
      }
    return -1;
}

