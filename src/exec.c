/*
** Copyright (C) 2002, 2003 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#if defined(HAVE_PATHS_H)
# include <paths.h>
#else
# define _PATH_DEVNULL "/dev/null"
#endif
#include <fcntl.h>
#include <sys/time.h>		/* FreeBSD wants this for the next one.. */
#include <sys/resource.h>
#include <signal.h>

#include "exec.h"
#include "term.h"

static char const rcsid[] = "@(#)$Id: exec.c,v 1.7 2003-05-03 23:22:40 kalt Exp $";

pid_t
exec(fd0, fd1, fd2, target, argv, timeout)
int *fd0, *fd1, *fd2;
u_int timeout;
char *target, **argv;
{
    int in[2], out[2], err[2];
    struct rlimit fdlimit;
    pid_t child;

    assert( fd1 != NULL );

    if (target != NULL)
      {
	static char *env = NULL;
	char *new;

	new = malloc(strlen(target) + 14);
	if (new == NULL)
	  {
	    eprint("malloc failed, cannot set SHMUX_TARGET to %s", target);
	    return -1;
	  }
	sprintf(new, "SHMUX_TARGET=%s", target);
	if (putenv(new) == -1)
	  {
	    eprint("putenv(%s) failed: %s", new, strerror(errno));
	    free(new);
	    return -1;
	  }
	if (env != NULL)
	    free(env);
	env = new;
      }
	
    if (fd0 != NULL) *fd0 = -1;
    *fd1 = -1;
    if (fd2 != NULL) *fd2 = -1;

    /* Get the pipes we need later on */
    in[0] = in[1] = -1;
    if (fd0 != NULL && pipe(in) == -1)
      {
	eprint("pipe(\"%s\"): %s", argv[0], strerror(errno));
	return -1;
      }
    if (pipe(out) == -1)
      {
	eprint("pipe(\"%s\"): %s", argv[0], strerror(errno));
	if (fd0 != NULL) { close(in[0]); close(in[1]); }
	return -1;
      }
    err[0] = err[1] = -1;
    if (fd2 != NULL && pipe(err) == -1)
      {
	eprint("pipe(\"%s\"): %s", argv[0], strerror(errno));
	if (fd0 != NULL) { close(in[0]); close(in[1]); }
	close(out[0]); close(out[1]);
	return -1;
      }

    /*
    ** Get the maximum number of open files for this process so we can
    ** cleanup things properly in the child.  The only reason this is
    ** done before forking is to be able to whine if the call fails.
    */
    if (getrlimit(RLIMIT_NOFILE, &fdlimit) == -1)
      {
	eprint("gerlimit(RLIMIT_NOFILE): %s", strerror(errno));
	fdlimit.rlim_cur = 1024;
      }

    /* fork() */
    child = fork();

    if (child == -1)
      {
	/* Nope.. */
	close(in[0]); close(in[1]); close(out[0]); close(out[1]);
	if (fd2 != NULL) { close(err[0]); close(err[1]); }
	eprint("fork(): %s", strerror(errno));
	return -1;
      }

    /* Let each process go its own way */
    if (child > 0)
      {
	/* Close the child side of the pipes */
	if (fd0 != NULL) { close(in[0]); *fd0 = in[1]; }
	close(out[1]); *fd1 = out[0];
	if (fd2 != NULL) { close(err[1]); *fd2 = err[0]; }
	return child;
      }
    else
      {
	struct sigaction sa;
	int fd;
	char error[1024];

	/*
	** Reset signal handlers as they are not appropriate for children.
	** Only SIGTSTP and SIGCONT really need to get reset as they are
	** used to notify the parent if something goes wrong.
	*/
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGCONT, &sa, NULL);
	sigaction(SIGWINCH, &sa, NULL);

	/* For simplicity later on */
	if (fd2 == NULL)
	    err[1] = out[1];
	/* Close all open filedescriptors */
	fd = 0;
	while (fd <= fdlimit.rlim_cur)
	  {
	    if (fd != in[0] && fd != out[1] && fd != err[1])
		close(fd);
	    fd += 1;
	  }
	assert( out[1] > 2 ); assert( err[1] > 2 );

	/* Setup the fds properly */
	if (fd0 != NULL)
	  {
	    if (dup(in[0]) == -1) abort();
	    if (close(in[0]) == -1) abort();
	  }
	else
	  {
	    in[0] = open(_PATH_DEVNULL, O_RDONLY, 0);
	    if (in[0] == -1)
	      {
		sprintf(error, "SHMUCK!\nopen(%s): %s\n",
			_PATH_DEVNULL, strerror(errno));
		/* The parent knows what to make of this */
		kill(getpid(), SIGTSTP);
		write(err[1], error, strlen(error)+1);
		_exit(0);
	      }
	    if (in[0] != 0)
	      {
		sprintf(error, "SHMUCK!\nopen(%s) returned %d\n",
			_PATH_DEVNULL, in[0]);
		/* The parent knows what to make of this */
		kill(getpid(), SIGTSTP);
		write(err[1], error, strlen(error)+1);
		_exit(0);
	      }
	  }
	if (dup(out[1]) == -1) abort();
	if (dup(err[1]) == -1) abort();
	if (close(out[1]) == -1) abort();
	if (fd2 != NULL && close(err[1]) == -1) abort();

	alarm(timeout);

	/* Finally, execvp() */
	execvp(argv[0], argv);

	/* 
	** This shouldn't happen, but if it does, let's get an error message
	** to the parent.
	*/
	alarm(0);
	sprintf(error, "SHMUCK!\nexecv(%s): %s\n", argv[0], strerror(errno));
	kill(getpid(), SIGTSTP); /* The parent knows what to make of this */
	write(2, error, strlen(error)+1);
	_exit(0);
      }
    /* NOTREACHED */
}
