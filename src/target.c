/*
** Copyright (C) 2002 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#include "target.h"
#include "term.h"

#include "status.h"

static char const rcsid[] = "@(#)$Id: target.c,v 1.4 2002-07-09 21:45:26 kalt Exp $";

extern char *myname;

struct target
{
    char *name;		/* target (host)name */
    int type;		/* 0: sh, 1: rsh, 2: sshv1, 3: sshv2, 4: ssh */
    char status;	/* -1: DEAD 0: unknown 1: ping okay 2: test rsh okay */
    char phase;		/* current phase:      1: ping      2: test   3: cmd */
};

static struct target *targets = NULL;
static int  type,	/* default type */
	    tcur = 0,	/* "current" target "pointer" */
	    tmax,	/* max target pointer */
	    tsz = 0;	/* size of targets array */

/*
** target_add
**	add a target to the list
*/
void
target_default(cmd)
char *cmd;
{
    if (strncmp("sh", cmd, 3) == 0)
	type = 0;
    else if (strncmp("rsh", cmd, 4) == 0)
	type = 1;
    else if (strncmp("ssh1", cmd, 5) == 0)
	type = 2;
    else if (strncmp("ssh2", cmd, 5) == 0)
	type = 3;
    else if (strncmp("ssh", cmd, 4) == 0)
	type = 4;
    else
      {
	fprintf(stderr, "%s: Unrecognized rcmd command: %s\n", myname, cmd);
	exit(1);
      }
}

/*
** target_add
**	add a target to the list
*/
int
target_add(name)
char *name;
{
    if (tsz == 0)
      {
	targets = (struct target *) malloc(sizeof(struct target) * 10);
	tsz = 10;
	tmax = -1;
      }
    else if (tmax+1 == tsz)
      {
	tsz *= 2;
	targets = (struct target *) realloc(targets,
					    sizeof(struct target) * tsz);
      }
    if (targets == NULL)
      {
	perror("malloc/realloc failed");
	exit(1);
      }

    tmax += 1;
    if (strncmp("sh:", name, 3) == 0)
      {
	targets[tmax].name = strdup(name+3);
	targets[tmax].type = 0;
      }
    else if (strncmp("rsh:", name, 4) == 0)
      {
	targets[tmax].name = strdup(name+4);
	targets[tmax].type = 1;
      }
    else if (strncmp("ssh1:", name, 5) == 0)
      {
	targets[tmax].name = strdup(name+5);
	targets[tmax].type = 2;
      }
    else if (strncmp("ssh2:", name, 5) == 0)
      {
	targets[tmax].name = strdup(name+5);
	targets[tmax].type = 3;
      }
    else if (strncmp("ssh:", name, 4) == 0)
      {
	targets[tmax].name = strdup(name+4);
	targets[tmax].type = 4;
      }
    else
      {
	targets[tmax].name = strdup(name);
	targets[tmax].type = type;
      }
    targets[tmax].status = 0;
    targets[tmax].phase = 0;

    return strlen(targets[tmax].name);
}

/*
** target_getmax
**	Return the number of targets.
*/
int
target_getmax(void)
{
    return tmax + 1;
}

/*
** target_setbyname
**	Find a target by name, and set the current pointer.
*/
int
target_setbyname(name)
char *name;
{
    assert( name != NULL );

    tcur = 0;
    while (tcur <= tmax && strcasecmp(targets[tcur].name, name) != 0)
	tcur += 1;
    if (tcur > tmax)
	return -1;
    return 0;
}

/*
** target_setbynum
**	Find a target by number, and set the current pointer.
*/
void
target_setbynum(num)
u_int num;
{
    if (num > tmax)
	abort();
    tcur = num;
}

/*
** target_getname
**	Return the current target name.
*/
char *
target_getname(void)
{
    assert( tcur >= 0 && tcur <= tmax );

    return targets[tcur].name;
}

/*
** target_getnum
**	Return the current target number.
*/
int
target_getnum(void)
{
    assert( tcur >= 0 && tcur <= tmax );

    return tcur;
}

/*
** target_getcmd
**	Return the current target command.
*/
void
target_getcmd(args, cmd)
char **args, *cmd;
{
    assert( tcur >= 0 && tcur <= tmax );

    switch (targets[tcur].type)
      {
      case 0:
	  args[0] = getenv("SHMUX_SH");
	  if (args[0] == NULL)
	      args[0] = "/bin/sh";
	  args[1] = "-c";
	  args[2] = cmd;
	  args[3] = NULL;
	  return;
      case 1:
	  args[0] = getenv("SHMUX_RSH");
	  if (args[0] == NULL)
	      args[0] = "rsh";
	  args[1] = "-n";
	  args[2] = targets[tcur].name;
	  args[3] = cmd;
	  args[4] = NULL;
	  return;
      case 2:
	  args[0] = getenv("SHMUX_SSH1");
	  args[1] = "-1n";
	  args[4] = getenv("SHMUX_SSH1_OPTS");
	  break;
      case 3:
	  args[0] = getenv("SHMUX_SSH2");
	  args[1] = "-2n";
	  args[4] = getenv("SHMUX_SSH2_OPTS");
	  break;
      case 4:
	  args[0] = NULL;
	  args[1] = "-n";
	  args[4] = NULL;
	  break;
      default:
	  abort();
      }

    /* Only ssh left */
    if (args[0] == NULL)
	args[0] = getenv("SHMUX_SSH");
    if (args[0] == NULL)
	args[0] = "ssh";
    args[2] = "-o";
    args[3] = "BatchMode=yes";
    if (args[4] == NULL)
	args[4] = getenv("SHMUX_SSH_OPTS");
    if (args[4] == NULL)
	args[4] = "-xa";
    if (args[4][0] == '\0')
      {
	args[4] = targets[tcur].name;
	args[5] = cmd;
	args[6] = NULL;
      }
    else
      {
	args[5] = targets[tcur].name;
	args[6] = cmd;
	args[7] = NULL;
      }
}

/*
** target_next
**	Find the next target available for a specific phase
**		1 -> Ping  <=>  status == 0
**		2 -> Test  <=>  status == 1
**		3 -> Exec  <=>  status == 2
**	Return 0 if there is such a target, -1 otherwise
*/
int
target_next(phase)
int phase;
{
    assert( phase > 0 && phase < 4 );

    tcur = 0;
    while (tcur <= tmax)
      {
	if (targets[tcur].status == phase-1
	    && targets[tcur].phase != phase)
	  {
	    targets[tcur].phase = phase;
	    return 0;
	  }
	tcur += 1;
      }
    return -1;
}

/*
** target_result
**	Set the result of the current phase for the current target.
*/
void
target_result(ok)
int ok;
{
    assert( tcur >= 0 && tcur <= tmax );
    assert( targets[tcur].status >= -1 && targets[tcur].status < 3 );
    assert( targets[tcur].phase > 0 && targets[tcur].phase <= 3 );

    status_phase(targets[tcur].status, -1);
    if (ok == 1)
	targets[tcur].status = targets[tcur].phase;
    else
	targets[tcur].status = -1;
    status_phase(targets[tcur].status, 1);
}


/*
** target_results
**	Show failures.
*/
void
target_results(void)
{
    int i, first;

    first = 1;
    i = 0;
    while (i <= tmax)
      {
	if (targets[i].status == -1)
	  {
	    if (first == 1)
		printf("Failed for: ");
	    first = 0;
	    printf("%s ", targets[i].name);
	  }
	i += 1;
      }
    nprint("");
}
