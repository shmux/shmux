/*
** Copyright (C) 2002 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>		/* FreeBSD wants this for the next one.. */
#include <sys/resource.h>
#include <poll.h>

#include "exec.h"
#include "loop.h"
#include "status.h"
#include "target.h"
#include "term.h"

static char const rcsid[] = "@(#)$Id: loop.c,v 1.6 2002-07-07 19:59:38 kalt Exp $";

struct child
{
    pid_t	pid;		/* Process ID */
    int		num;		/* target number */
    int		test, passed;	/* test?, passed? */
    int		execstate;	/* exec() status: 0=ok, 1=failed? 2=failed */
    char	*obuf, *ebuf;	/* stdout/stderr truncated buffer */
    int		status;		/* waitpid(status) */
};

static void init_child(struct child *);
static void parse_child(char *, int, int, struct child *, int, char *);
static void parse_fping(char *);

/*
** init_child
**	Used to initnialize a child structure whenever a new child is spawned.
*/
static void
init_child(kid)
struct child *kid;
{
    kid->num = target_getnum();
    kid->test = kid->passed = 0;
    kid->execstate = 0;
    kid->obuf = kid->ebuf = NULL;
    kid->status = -1;

    status_spawned(1);
}

/*
** parse_child
**	Parse output from children
*/
static void
parse_child(name, isfping, verbose_tests, kid, std, buffer)
char *name, *buffer;
int isfping, verbose_tests, std;
struct child *kid;
{
    char *start, *nl;

    assert( std == 1 || std == 2 );

    start = nl = buffer;

    while (*nl != '\0')
      {
	char **left;

	if (*nl != '\n') /* Purify reports *1* UMR here?!? */
	  {
	    nl += 1;
	    continue;
	  }

	if (*(nl-1) == '\r') /* XXX */
	    *(nl-1) = '\0';
	else
	    *nl = '\0';

	left = NULL;
	/* Check which state the child is in. */
	switch (kid->execstate)
	  {
	  case 2:
	      /* Error from exec() */
	      eprint("Fatal error for %s: %s", name, start);
	      break;
	  case 1:
	      /* Possible error message from exec() */
	      if (strcmp(start, "SHMUCK!") == 0)
		{
		  /* Should be from exec() */
		  kid->execstate = 2;
		  break;
		}
	      /* Can't be.. */
	      eprint("Unexpected meaningless SIGTSTP received by child spawned for '%s'.  Recovering..", name);
	      kid->execstate = 0;
	      /* FALLTHROUGH */
	  case 0:
	      /* General case, we read output from child */
	      if (std == 1)
		  left = &(kid->obuf); /* stdout */
	      else
		  left = &(kid->ebuf); /* stderr */
	      
	      if (isfping == 0)
		{
		  /* Test or real command */
		  if (kid->test == 1)
		    {
		      /*
		      ** `SHMUX.' must arrive all at once (including the \n).
		      ** Considered reasonnable until proven to be a bug.
		      */
		      if (strcmp(start, "SHMUX.") == 0
			  && kid->passed == 0 && std == 1)
			  kid->passed = 1;
		      else
			  kid->passed = -1;

		      if (verbose_tests == 1 && kid->passed == -1)
			  eprint("Test output for %s: %s%s",
				 name, (*left == NULL) ? "" : *left, start);
		      else
			  dprint("Test output for %s: %s%s",
				 name, (*left == NULL) ? "" : *left, start);
		    }
		  else
		    {
		      if (kid->test == 1)
			  kid->passed = -1;
		      tprint(name, ((std == 1) ? MSG_STDOUT : MSG_STDERR),
			     "%s%s", (*left == NULL) ? "" : *left, start);
		    }
		}
	      else
		  parse_fping(start);

	      break;
	  default:
	      abort();
	  }
	
	start = nl += 1;
	if (left != NULL && *left != NULL)
	  {
	    free(*left);
	    *left = NULL;
	  }
      }

    if (start != nl)
      {
	/* There is some leftover data not terminated by \n */
	assert( start < nl );
	if (isfping == 0)
	  {
	    char **left;

	    if (std == 1)
		left = &(kid->obuf); /* stdout */
	    else
		left = &(kid->ebuf); /* stderr */

	    if (*left != NULL)
	      {
		/*
		** This is a little weak, but then again it shouldn't happen
		** either..  Who just said "Never say never!"?  i swear i
		** just heard someone say that...
		*/
		tprint(name, ((std == 1) ? MSG_STDOUTTRUNC : MSG_STDERRTRUNC),
		       "%s", *left);
		free(*left);
	      }
	    *left = strdup(start);
	  }
	else
	  {
	    assert( isfping == 1 );
	    eprint("Truncated output from fping lost: %s", start);
	  }
      }
}

/*
** parse_fping
**	Parse one line of output from fping
*/
static void
parse_fping(line)
char *line;
{
    char *space;
		  
    space = strchr(line, ' ');
    if (space != NULL)
      {
	*space = '\0';
	if (target_setbyname(line) != 0)
	  {
	    *space = ' ';
	    dprint("fping garbage follows:");
	    eprint("%s", line);
	  }
	else
	  {
	    *space = ' ';
	    if (strcmp(space+1, "is alive") == 0)
	      {
		iprint("%s", line);
		target_result(1);
	      }
	    else
	      {
		/* Assuming too much? */
		eprint("%s", line);
		target_result(0);
	      }
	  }
      }
    else if (strcmp(line, "") != 0)
      {
	dprint("fping garbage follows:");
	eprint("%s", line);
      }
}

/*
** loop
**	Main loop.  Takes care of (optionally) pinging targets, testing
**	targets with a simple echo command, and finally running a command.
*/
void
loop(cmd, ctimeout, max, ping, test)
char *cmd, *ping;
int max;
u_int ctimeout, test;
{
    struct rlimit fdlimit;
    struct child *children;
    struct pollfd *pfd;
    char *cargv[10];

    /*
    ** Since there's a limit on the number of open file descriptors a
    ** process may have, we do some math and try to avoid running into
    ** it which could be unpleasant depending on what we're trying to
    ** achieve when we run out.  The assumptions are:
    ** + 3 for stdin, stdout and stderr (our own)
    ** + 3 for stdin, stdout and stderr for fping
    ** + 3 for pipe creation in exec.c/exec()
    ** + (3 * max) for children stdin, stdout and stderr
    ** And we add another 10 as safety margin.
    */  
    if (getrlimit(RLIMIT_NOFILE, &fdlimit) == -1)
      {
	eprint("getrlimit(RLIMIT_NOFILE): %s", strerror(errno));
	exit(1);
      }
    if (fdlimit.rlim_cur < (max + 3) * 3 + 10)
      {
	fdlimit.rlim_cur = (max + 3) * 3 + 10;
	if (fdlimit.rlim_cur > fdlimit.rlim_max)
	    fdlimit.rlim_cur = fdlimit.rlim_max;

	if (setrlimit(RLIMIT_NOFILE, &fdlimit) == -1)
	    eprint("setrlimit(RLIMIT_NOFILE, %d): %s",
		   (int) fdlimit.rlim_cur, strerror(errno));

	if (getrlimit(RLIMIT_NOFILE, &fdlimit) == -1)
	  {
	    eprint("getrlimit(RLIMIT_NOFILE): %s", strerror(errno));
	    eprint("Unable to validate parallelism factor.");
	  }
	else if (fdlimit.rlim_cur < (max + 3) * 3 + 10)
	  {
	    int old;
	    
	    old = max;
	    max = ((fdlimit.rlim_cur - 10) / 3) - 3;
	    eprint("Reducing parallelism factor to %d (from %d) because of system limitation.", max, old);
	  }
      }
	
    /* Allocate the control structures */
    pfd = (struct pollfd *) malloc((max+2)*3 * sizeof(struct pollfd));
    if (pfd == NULL)
      {
	perror("malloc/realloc failed");
	exit(1);
      }
    memset((void *) pfd, 0, (max+2)*3 * sizeof(struct pollfd));
    children = (struct child *) malloc((max+1) * sizeof(struct child));
    if (children == NULL)
      {
	perror("malloc/realloc failed");
	exit(1);
      }
    memset((void *) children, 0, (max+1)*sizeof(struct child));

    /* Initialize the status module. */
    status_init(ping != NULL, test != 0);

    /* Run fping if requested */
    if (ping != NULL)
      {
	u_int count = 0;
	
	pfd[2].fd = -1;
	cargv[0] = "fping"; cargv[1] = "-t"; cargv[2] = ping; cargv[3] = NULL;
	children[0].pid = exec(&(pfd[0].fd), &(pfd[1].fd), &(pfd[2].fd),
			       NULL, cargv, 0);
	if (children[0].pid == -1)
	    /* Error message was given by exec() */
	    exit(1);
	init_child(&(children[0]));

	pfd[1].events = POLLIN;
	pfd[2].events = POLLIN;

	while (target_next(1) == 0)
	  {
	    count += 1;
	    write(pfd[0].fd, target_getname(), strlen(target_getname()));
	    write(pfd[0].fd, "\n", 1);
	  }
	close(pfd[0].fd); pfd[0].fd = -1;
	iprint("Pinging %u targets...", count);
	dprint("fping pid = %d (idx=0) %d/%d/%d",
	       children[0].pid, pfd[0].fd, pfd[1].fd, pfd[2].fd);
	ping = NULL;
      }
    else
	while (target_next(1) == 0)
	    target_result(1);

    /* From here on, it's one big loop. */
    while (1)
      {
	int pollrc, idx, done;
	char *what;

	/* Update the status line before (possibly) pausing in poll() */
	status_update();
	/* Check for data to read/write */
	pollrc = poll(pfd, (max+2)*3, 250);
	if (pollrc == -1 && errno != EINTR)
	  {
	    perror("poll");
	    exit(1);
	  }

	if (pollrc > 0)
	  {
	    dprint("poll(%d) = %d", (max+2)*3, pollrc);
	    idx = 0;
	    while (idx < (max+2)*3)
	      {
		if (pfd[idx].fd == -1 || pfd[idx].revents == 0)
		  {
		    idx +=1;
		    continue;
		  }

		if (idx < 3)
		    what = "fping";
		else
		  {
		    target_setbynum(children[idx/3].num);
		    what = target_getname();
		  }

		/* Something is going on.. */
		dprint("idx=%d[%s] fd=%d(%d) IN=%d OUT=%d ERR=%d HUP=%d",
		       idx, what, pfd[idx].fd, idx%3,
		       (pfd[idx].revents & POLLIN) != 0,
		       (pfd[idx].revents & POLLOUT) != 0,
		       (pfd[idx].revents & POLLERR) != 0,
		       (pfd[idx].revents & POLLHUP) != 0);

		if (idx % 3 != 0)
		  {
		    /* Stdout or stderr with output ready to be read */
		    char buffer[8192];
		    int sz;

		    sz = read(pfd[idx].fd, buffer, 8191);
		    dprint("idx=%d[%s] fd=%d(%d) read()=%d",
			   idx, what, pfd[idx].fd, idx%3, sz);
		    if (sz > 0)
		      {
			buffer[sz] = '\0';
			parse_child(what, idx<=2, test<0, children+(idx/3),
				    idx%3, buffer);
		      }
		    else
		      {
			char **left;

			/*
			** Child is probably gone, we'll catch that below;
			** For now, just cleanup.
			*/
			if (sz == -1)
			    eprint("Unexpected read(STD%s) error for %s: %s",
				   (idx%3 == 1) ? "OUT" : "ERR", what,
				   strerror(errno));
			close(pfd[idx].fd); pfd[idx].fd = -1;
			if (idx%3 == 1)
			    left = &(children[idx/3].obuf);
			else
			    left = &(children[idx/3].ebuf);
			if (*left != NULL)
			  {
			    tprint(what, (idx%3 == 1) ? MSG_STDOUTTRUNC
				   : MSG_STDERRTRUNC, "%s", *left);
			    eprint("Previous line was incomplete."); /* So? */
			    free(*left);
			    *left = NULL;
			  }
		      }
		  }
		else
		  {
		    /* Stdin ready to be written to */
		    abort(); /* we don't use this yet, so did we get here? */
		  }

		idx += 1;
	      }
	  }

	/* Check for dead children */
	idx = 0; done = 1;
	while (idx < max+1)
	  {
	    int status, wprc;

	    if (children[idx].pid <= 0)
	      {
		/* Available slot to spawn a new child */
		if (idx > 0 && target_next(3) == 0)
		  {
		    done = 0;
		    pfd[idx*3].fd = -1;
		    target_getcmd(cargv, cmd);
		    children[idx].pid = exec(NULL, &(pfd[idx*3+1].fd),
					     &(pfd[idx*3+2].fd),
					     target_getname(), cargv,
					     ctimeout);
		    if (children[idx].pid == -1)
			  {
			    /* Error message was given by exec() */
			    eprint("Fatal error for %s", target_getname());
			    target_result(-1);
			    continue;
			  }

		    pfd[idx*3+1].events = POLLIN;
		    pfd[idx*3+2].events = POLLIN;

		    init_child(&(children[idx]));
		    dprint("%s, phase 3: pid = %d (idx=%d) %d/%d/%d",
			   target_getname(), children[idx].pid, idx,
			   pfd[idx*3].fd, pfd[idx*3+1].fd, pfd[idx*3+2].fd);
		    idx += 1;
		    continue;
		  }

		if (idx > 0 && children[idx].pid <= 0 && target_next(2) == 0)
		  {
		    done = 0;
		    if (test != 0)
		      {
			pfd[idx*3].fd = -1;
			target_getcmd(cargv, "echo SHMUX.");
			children[idx].pid = exec(NULL, &(pfd[idx*3+1].fd),
						 &(pfd[idx*3+2].fd),
						 target_getname(),
						 cargv, abs(test));

			if (children[idx].pid == -1)
			  {
			    /* Error message was given by exec() */
			    eprint("Fatal error for %s", target_getname());
			    target_result(-1);
			    continue;
			  }

			pfd[idx*3+1].events = POLLIN;
			pfd[idx*3+2].events = POLLIN;

			init_child(&(children[idx]));
			children[idx].test = 1;
			dprint("%s, phase 2: pid = %d (idx=%d) %d/%d/%d",
			       target_getname(), children[idx].pid, idx,
			       pfd[idx*3].fd, pfd[idx*3+1].fd,pfd[idx*3+2].fd);
			idx += 1;
			continue;
		      }
		    else
		      {
			target_result(1);
			dprint("%s skipped test", target_getname());
		      }
		  }

		/* Nothing left to do! */
		idx += 1;
		continue;
	      }

	    done = 0;
	    if (idx == 0)
		what = "fping";
	    else
	      {
		target_setbynum(children[idx].num);
		what = target_getname();
	      }
	    
	    if (children[idx].status >= 0)
	      {
		wprc = children[idx].pid;
		status = children[idx].status;
	      }
	    else
		wprc = waitpid(children[idx].pid, &status, WNOHANG|WUNTRACED);

	    if (wprc <= 0)
	      {
		if (wprc == -1)
		    eprint("waitpid(%d[%s]): %s",
			   children[idx].pid, what, strerror(errno));
		idx += 1;
		continue;
	      }

	    if (WIFSTOPPED(status) != 0)
	      {
		/*
		** YYY These could/should be ignored once we've received
		** some output from the child.
		*/
		if (WSTOPSIG(status) == SIGTSTP)
		  {
		    /* exec() failed, see exec.c */
		    dprint("%s (idx=%d) stopped on SIGTSTP, sending SIGCONT.",
			   what, idx);
		    children[idx].execstate = 1;
		    kill(children[idx].pid, SIGCONT);
		  }
		else
#if defined(SYS_SIGLIST_DECLARED)
		    eprint("%s for %s stopped on SIG%s!?",
			   (children[idx].test == 0) ? "Child" : "Test", what,
			   sys_signame[WSTOPSIG(status)]);
#else
		    eprint("%s for %s stopped on signal %d!?",
			   (children[idx].test == 0) ? "Child" : "Test", what,
			   WSTOPSIG(status));
#endif
		idx += 1;
		continue;
	      }

	    /* XXX what about +0 ?  needed here or not? need to test! */
	    if (pfd[idx*3+1].fd != -1 || pfd[idx*3+2].fd !=-1)
	      {
		/* Let's finish reading from these before going on */
		if (children[idx].status == -1)
		    dprint("%s (idx=%d) died but has open fd(s), saved status",
			   what, idx);
		children[idx].status = status;
		idx += 1;
		continue;
	      }

	    if (pfd[idx*3].fd != -1)
	      {
		/* XXX */
		close(pfd[idx*3].fd); pfd[idx*3].fd = -1;
	      }

	    if (WIFEXITED(status) != 0)
	      {
		if (children[idx].test == 1)
		    dprint("Test for %s exited with status %d",
			   what, WEXITSTATUS(status));
		else
		  {
		    if (WEXITSTATUS(status) != 0
			&& children[idx].execstate == 0
			&& (idx > 0 || WEXITSTATUS(status) > 2))
			eprint("Child for %s exited with status %d",
			       what, WEXITSTATUS(status));
		    else if (idx > 0 && children[idx].execstate == 0)
			iprint("Child for %s exited (with status %d)",
			       what, WEXITSTATUS(status));
		  }
	      } else {
		assert( WTERMSIG(status) != 0 );
		if (WTERMSIG(status) == SIGALRM)
		    if (children[idx].test == 0)
			eprint("Child for %s timed out", what);
		    else
			children[idx].passed = -2;
		else
#if defined(SYS_SIGLIST_DECLARED)
		    eprint("%s for %s died on SIG%s%s",
			   (children[idx].test == 0) ? "Child" : "Test", what,
			   sys_signame[WTERMSIG(status)],
			   (WCOREDUMP(status) != 0) ? " (core dumped)" : "");
#else
		    eprint("%s for %s died on signal %d%s",
			   (children[idx].test == 0) ? "Child" : "Test", what,
			   WTERMSIG(status),
			   (WCOREDUMP(status) != 0) ? " (core dumped)" : "");
#endif
	      }

	    children[idx].pid = 0;

	    if (idx == 0)
		dprint("fping is done");
	    else
	      {
		target_setbynum(children[idx].num);
		if (children[idx].execstate != 0
		    || (children[idx].test == 1 && children[idx].passed != 1))
		  {
		    if (children[idx].test == 1)
			eprint("Test %s for %s",
			       (children[idx].passed == -2) ? "timed out"
			       : "failed", what);
		    target_result(-1);
		  }
		else
		    target_result(1);
	      }

	    status_spawned(-1);

	    /* Don't increment, catch it at the top again */
	  }

	if (done == 1)
	    break;
      }

    free(children);
    free(pfd);

    sprint("");
}
