/*
** Copyright (C) 2002 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#include <libgen.h>
#include <dirent.h>
#if defined(HAVE_PATHS_H)
# include <paths.h>
#else
# define _PATH_TMP "/tmp"
#endif
#include <time.h>
#include <sys/stat.h>

#include "version.h"

#include "loop.h"
#include "target.h"
#include "term.h"
#include "units.h"

static char const rcsid[] = "@(#)$Id: shmux.c,v 1.12 2002-08-09 14:50:35 kalt Exp $";

extern char *optarg;
extern int optind, opterr;

char *myname;

#define	DEFAULT_MAXWORKERS 10
#define DEFAULT_PINGTIMEOUT "500"
#define DEFAULT_TESTTIMEOUT 15
#define DEFAULT_RCMD "ssh"

static void usage(int);

static void
usage(detailed)
int detailed;
{
    fprintf(stderr, "Usage: %s [ options ] -c <command> <target1> [ <target2> ... ]\n", myname);
/*    fprintf(stderr, "Usage: %s [ options ] -i [ <target1> [ <target2> ... ] ]\n", myname);*/
    if (detailed == 0)
	return;
    fprintf(stderr, "  -h            Print this message.\n");
    fprintf(stderr, "  -V            Output version info.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -c <command>  Command to execute on targets.\n");
    fprintf(stderr, "  -C <timeout>  Set a command timeout.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -M            Maximum number of simultaneous processes (Default: %u).\n", DEFAULT_MAXWORKERS);
    fprintf(stderr, "  -r <rcmd>     Set the default method (Default: %s).\n", DEFAULT_RCMD);
    fprintf(stderr, "  -p            Ping targets to check for life.\n");
    fprintf(stderr, "  -P <millisec> Initial target timeout given to fping (Default: %s).\n", DEFAULT_PINGTIMEOUT);
    fprintf(stderr, "  -t            Send test command to verify target health.\n");
    fprintf(stderr, "  -T <seconds>  Time to wait for test answer (Default: %d).\n", DEFAULT_TESTTIMEOUT);
    fprintf(stderr, "\n");
    fprintf(stderr, "  -o <dir>      Send the output to files under the specified directory.\n");
    fprintf(stderr, "  -m            Don't mix target outputs.\n");
    fprintf(stderr, "  -b            Show bare output without target names.\n");
    fprintf(stderr, "  -s            Suppress progress status.\n");
    fprintf(stderr, "  -q            Suppress final summary.\n");
    fprintf(stderr, "  -v            Display internal status messages.\n");
    fprintf(stderr, "  -D            Display internal debug messages.\n");
}

int
main(int argc, char **argv)
{
    int badopt;
    int opt_prefix, opt_status, opt_quiet, opt_internal, opt_debug;
    int opt_ctimeout, opt_mixed, opt_maxworkers, opt_vtest;
    u_int opt_test;
    char *opt_rcmd, *opt_command, *opt_odir, *opt_ping, tdir[MAXNAMLEN];
    int longest, ntargets;
    time_t start;

    myname = basename(argv[0]);

    opt_prefix = opt_status = 1;
    opt_quiet = opt_internal = opt_debug = 0;
    opt_mixed = 1;
    opt_maxworkers = DEFAULT_MAXWORKERS;
    opt_ctimeout = opt_test = opt_vtest = 0;
    opt_rcmd = getenv("SHMUX_RCMD");
    if (opt_rcmd == NULL)
	opt_rcmd = DEFAULT_RCMD;
    opt_command = opt_odir = opt_ping = NULL;

    badopt = 0;
    while (1)
      {
        int c;
	
        c = getopt(argc, argv, "bc:C:DhmM:o:pP:qr:stT:vV");
	
        /* Detect the end of the options. */
        if (c == -1)
            break;
	
        switch (c)
          {
	  case 'b':
	      opt_prefix = 0;
	      break;
	  case 'c':
	      opt_command = optarg;
	      break;
	  case 'C':
	      opt_ctimeout = unit_time(optarg);
	      break;
	  case 'D':
	      opt_debug = 1;
	      break;
          case 'h':
              usage(1);
              exit(0);
              break;
	  case 'm':
	      opt_mixed = 0;
	      break;
	  case 'M':
	      opt_maxworkers = atoi(optarg);
	      break;
	  case 'o':
	      opt_odir = optarg;
	      break;
	  case 'p':
	      if (opt_ping == NULL)
		  opt_ping = DEFAULT_PINGTIMEOUT;
	      break;
	  case 'P':
	      opt_ping = optarg;
	      break;
	  case 'q':
	      opt_quiet = 1;
	      break;
	  case 'r':
	      opt_rcmd = optarg;
	      break;
	  case 's':
	      opt_status = 0;
	      break;
	  case 't':
	      if (opt_test == 0)
		  opt_test = DEFAULT_TESTTIMEOUT;
	      opt_vtest += 1;
	      break;
	  case 'T':
	      opt_test = atoi(optarg);
	      opt_vtest += 1;
	      break;
	  case 'v':
	      opt_internal = 1;
	      break;
	  case 'V':
	      printf("%s version %s\n", myname, SHMUX_VERSION);
	      exit(0);
	  case '?':
	      badopt += 1;
	      break;
	  default:
	      abort();
	  }
      }

    if (optind >= argc || badopt > 0 || opt_command == NULL)
      {
        usage(0);
        exit(1);
      }

    target_default(opt_rcmd);
    if (opt_maxworkers <= 0)
      {
	fprintf(stderr, "%s: Invalid -M argument!\n", myname);
	exit(1);
      }

    if (opt_mixed == 0 && opt_odir == NULL)
      {
	char *tmp;

	tmp = getenv("TMPDIR");
	sprintf(tdir, "%s/%s.%d.%ld", (tmp != NULL) ? tmp : _PATH_TMP, myname,
		(int) getpid(), (long) time(NULL));
	opt_odir = tdir;
      }
    else
	opt_mixed = 1;

    if (opt_odir != NULL && mkdir(opt_odir, 0777) == -1 && errno != EEXIST)
      {
	fprintf(stderr, "%s: mkdir(%s): %s\n",
		myname, opt_odir, strerror(errno));
	exit(1);
      }

    if (opt_vtest > 1)
	opt_test *= -1;

    ntargets = 0;
    longest = strlen(myname);
    while (optind < argc)
      {
	int length;

	length = target_add(argv[optind++]);
	if (length > longest)
	    longest = length;
	ntargets += 1;
      }

    term_init(longest, opt_prefix, opt_status, opt_internal, opt_debug);

    start = time(NULL);
    loop(opt_command, opt_ctimeout, opt_maxworkers,
	 opt_mixed, opt_odir, opt_ping, opt_test);

    if (opt_mixed == 0 && rmdir(opt_odir) == -1)
	&& rmdir(opt_odir) == -1)
      {
	fprintf(stderr, "%s: rmdir(%s): %s\n",
		myname, opt_odir, strerror(errno));
	exit(1);
      }
    /* Summary of results unless asked to be quiet */
    if (opt_quiet == 0)
      {
	nprint("%d targets processed in %d seconds.",
	       ntargets, (int) (time(NULL) - start));
	target_results();
	target_results((int) (time(NULL) - start));
      }

    exit(0);
}
