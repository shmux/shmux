/*
** Copyright (C) 2002, 2003 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#include "target.h"
#include "term.h"

#include "status.h"

static char const rcsid[] = "@(#)$Id: target.c,v 1.10 2003-04-26 01:39:00 kalt Exp $";

extern char *myname;

struct target
{
    char *name;		/* target (host)name */
    int type;		/* 0: sh, 1: rsh, 2: sshv1, 3: sshv2, 4: ssh */
    			/* code	current phase	status
			** -1	N/A		DEAD
			**  0   N/A		Unknown
			**  1	ping		ping okay
			**  2	test		test okay
			**  3	cmd		cmd done, exit status okay
			**  4   analyzer	all done
			*/    
    char status;
    char phase;
    int result;		/* command status: -2: signal, -1: timed out,
			   		    0: unknown, 1: ok, 2: error */
};

static struct target *targets = NULL;
static int  type,	/* default type */
	    tcur = 0,	/* "current" target "pointer" */
	    tmax,	/* max target pointer */
	    tsz = 0;	/* size of targets array */

/*
** target_default
**	Configure default method
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
    targets[tmax].result = 0;

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
int
target_setbynum(num)
u_int num;
{
    if (num > tmax)
	return -1;
    tcur = num;
    return 0;
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
**		1 -> Ping  	<=>  status == 0
**		2 -> Test  	<=>  status == 1
**		3 -> Exec  	<=>  status == 2
**		4 -> Analyzer	<=>  status == 3
**	Return 0 if there is such a target, -1 otherwise
*/
int
target_next(phase)
int phase;
{
    assert( phase > 0 && phase < 5 );

    tcur = 0;
    while (tcur <= tmax)
      {
	if (targets[tcur].status == phase-1
	    /* && targets[tcur].status==targets[tcur].phase true,unnecessary */
	    && targets[tcur].phase != phase)
	    return 0;
	tcur += 1;
      }
    return -1;
}

/*
** target_start
**	Start new phase for current target.
*/
void
target_start(void)
{
    assert( tcur >= 0 && tcur <= tmax );
    assert( targets[tcur].status == targets[tcur].phase );
    assert( targets[tcur].phase >= 0 && targets[tcur].phase < 4 );

    targets[tcur].phase = targets[tcur].phase + 1;
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
    assert( targets[tcur].status >= -1 && targets[tcur].status < 4 );
    assert( targets[tcur].phase > 0 && targets[tcur].phase <= 4 );

    status_phase(targets[tcur].status, -1);
    if (ok == 1)
      {
	if (targets[tcur].result == CMD_ERROR)
	  {
	    assert( targets[tcur].phase >= 3 );
	    targets[tcur].phase = 4;
	  }
	targets[tcur].status = targets[tcur].phase;
      }
    else
      {
	targets[tcur].status = -1;
	targets[tcur].result = CMD_FAILURE;
      }
    status_phase(targets[tcur].status, 1);
}

/*
** target_cmdstatus
**	Set the result of the command execution for the current target.
*/
void
target_cmdstatus(status)
int status;
{
    assert( tcur >= 0 && tcur <= tmax );
    assert( targets[tcur].phase == 3 || targets[tcur].phase == 4 );
    assert( status >= -2 && status <= 2 );

    targets[tcur].result = status;
}

/*
** target_status
**	Report current status of all targets
*/
void
target_status(status)
int status;
{
    int i, any, tlen;
    char buf[16];

    assert( status == STATUS_ALL     || status == STATUS_PENDING ||
	    status == STATUS_ACTIVE  || status == STATUS_FAILED  ||
	    status == STATUS_ERROR   || status == STATUS_SUCCESS );

    tlen = sprintf(buf, "%u", tmax);
    any = 0;
    i = 0;
    while (i <= tmax)
      {
	if (targets[i].result < 0 && (status & STATUS_FAILED) != 0)
	  {
	    assert( targets[i].result == CMD_FAILURE
		    || targets[i].result == CMD_TIMEOUT );
	    uprint(" [%*d] %s: %s", tlen, i,
		   (targets[i].result == CMD_FAILURE) ?
		   "            failed" : "         timed out",
		   targets[i].name);
	    any = 1;
	  }
	else if (targets[i].result == CMD_ERROR
		 && (status & STATUS_ERROR) != 0)
	  {
	    uprint(" [%*d]             error: %s", tlen, i, targets[i].name);
	    any = 1;
	  }
	else if (targets[i].result == CMD_SUCCESS
		 && (status & STATUS_SUCCESS) != 0)
	  {
	    uprint(" [%*d]           success: %s", tlen, i, targets[i].name);
	    any = 1;
	  }
	else if (targets[i].status != targets[i].phase
		 && (status & STATUS_ACTIVE) != 0)
	  {
	    char *what;

	    switch (targets[i].phase)
	      {
	      case 1:
		  what = "  [pinging] active";
		  break;
	      case 2:
		  what = "  [testing] active";
		  break;
	      case 3:
		  what = "  [running] active";
		  break;
	      case 4:
		  what = "[analyzing] active";
		  break;
	      default:
		  abort();
	      }

	    uprint(" [%*d]%s: %s", tlen, i, what, targets[i].name);
	    any = 1;
	  }
	else if (targets[i].phase < 3
		 && (status & STATUS_PENDING) != 0)
	  {
	    uprint(" [%*d]           pending: %s", tlen, i, targets[i].name);
	    any = 1;
	  }
	i += 1;
      }

    if (any == 0)
	uprint("no such target.");
}

/*
** target_results
**	Show failures.
*/
void
target_results(seconds)
int seconds;
{
    int i, first;
    int f, t, u, s, e;

    f = t = u = s = e = 0;
    i = 0;
    while (i <= tmax)
      {
	switch (targets[i].result)
	  {
	  case -2:
	      f += 1;
	      break;
	  case -1:
	      t += 1;
	      break;
	  case 0:
	      u += 1;
	      break;
	  case 1:
	      s += 1;
	      break;
	  case 2:
	      e += 1;
	      break;
	  default:
	      eprint("Unknown target result found!");
	      break;
	  }
	i += 1;
      }

    if (seconds >= 0)
	nprint("%d target%s processed in %d second%s.",
	       tmax + 1, (tmax > 0) ? "s" : "",
	       seconds, (seconds > 1) ? "s" : "");
    else
	nprint("%d target%s processed.", tmax + 1, (tmax > 0) ? "s" : "");

    if (f + t + u + s + e > 0)
      {
	printf("Summary: ");
	if (f > 0)
	    printf("%d failure%s", f, (f > 1) ? "s" : "");
	if (f > 0 && t + u + s + e > 0)
	    printf(", ");
	if (t > 0)
	    printf("%d timeout%s", t, (t > 1) ? "s" : "");
	if (t > 0 && u + s + e > 0)
	    printf(", ");
	if (u > 0)
	    printf("%d unprocessed", u);
	if (u > 0 && s + e > 0)
	    printf(", ");
	if (s > 0)
	    printf("%d success%s", s, (s > 1) ? "es" : "");
	if (e > 0)
	  {
	    if (s > 0)
		printf(", ");
	    printf("%d error%s", e, (e > 1) ? "s" : "");
	  }
	nprint("");
      }

    first = 1;
    i = 0;
    while (i <= tmax)
      {
	if (targets[i].result == CMD_FAILURE)
	  {
	    if (first == 1)
		printf("Failed   : ");
	    first = 0;
	    printf("%s ", targets[i].name);
	  }
	i += 1;
      }
    nprint("");

    first = 1;
    i = 0;
    while (i <= tmax)
      {
	if (targets[i].result == CMD_TIMEOUT)
	  {
	    if (first == 1)
		printf("Timed out: ");
	    first = 0;
	    printf("%s ", targets[i].name);
	  }
	i += 1;
      }
    nprint("");

    first = 1;
    i = 0;
    while (i <= tmax)
      {
	if (targets[i].result == CMD_ERROR)
	  {
	    if (first == 1)
		printf("Error    : ");
	    first = 0;
	    printf("%s ", targets[i].name);
	  }
	i += 1;
      }
    nprint("");
}
