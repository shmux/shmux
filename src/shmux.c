/*
** Copyright (C) 2002, 2003 Christophe Kalt
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
#if defined(HAVE_PCRE_H)
# include <pcre.h>
#endif

#include "version.h"

#include "analyzer.h"
#include "byteset.h"
#include "loop.h"
#include "target.h"
#include "term.h"
#include "units.h"

static char const rcsid[] = "@(#)$Id: shmux.c,v 1.27 2003-11-08 01:17:28 kalt Exp $";

extern char *optarg;
extern int optind, opterr;

char *myname;

/* Miscellaneous default settings */
#define DEFAULT_ANALYSIS "regex"
#define DEFAULT_ERRORCODES "1-"
#define	DEFAULT_MAXWORKERS 10
#define DEFAULT_PINGTIMEOUT "500"
#define DEFAULT_RCMD "ssh"
#define DEFAULT_SPAWNMODE "one"
#define DEFAULT_TESTTIMEOUT 15

static void usage(int);

static void
usage(detailed)
int detailed;
{
    fprintf(stderr, "Usage: %s [ options ] -c <command> [ - | <target1> [ <target2> ... ] ]\n", myname);
/*    fprintf(stderr, "Usage: %s [ options ] -i [ <target1> [ <target2> ... ] ]\n", myname);*/
    if (detailed == 0)
	return;
    fprintf(stderr, "  -h            Print this message.\n");
    fprintf(stderr, "  -V            Output version info.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -c <command>  Command to execute on targets.\n");
    fprintf(stderr, "  -C <timeout>  Set a command timeout.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -M <max>      Maximum number of simultaneous processes (Default: %u).\n", DEFAULT_MAXWORKERS);
    fprintf(stderr, "  -r <rcmd>     Set the default method (Default: %s).\n", DEFAULT_RCMD);
    fprintf(stderr, "  -p            Ping targets to check for life.\n");
    fprintf(stderr, "  -P <millisec> Initial target timeout given to fping (Default: %s).\n", DEFAULT_PINGTIMEOUT);
    fprintf(stderr, "  -t            Send test command to verify target health.\n");
    fprintf(stderr, "  -T <seconds>  Time to wait for test answer (Default: %d).\n", DEFAULT_TESTTIMEOUT);
    fprintf(stderr, "\n");
    fprintf(stderr, "  -S <mode>     Spawn strategy (Default: \"%s\")\n", DEFAULT_SPAWNMODE);
    fprintf(stderr, "  -e <range>    Exit codes to consider errors (Default: \"%s\")\n", DEFAULT_ERRORCODES);
    fprintf(stderr, "  -E <range>    Exit codes to always display (Default: \"%s\")\n", DEFAULT_ERRORCODES);
    fprintf(stderr, "  -a <type>     Analysis type (Default: %s)\n", DEFAULT_ANALYSIS);
    fprintf(stderr, "  -A <test>     Analyze output to determine success from failure.\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  -o <dir>      Send the output to files under the specified directory.\n");
    fprintf(stderr, "  -m            Don't mix target outputs.\n");
    fprintf(stderr, "  -b            Show bare output without target names.\n");
    fprintf(stderr, "  -s            Suppress progress status.\n");
    fprintf(stderr, "  -q            Suppress output of successful targets.\n");
    fprintf(stderr, "  -qq           Suppress target output.\n");
    fprintf(stderr, "  -Q            Suppress final summary.\n");
    fprintf(stderr, "  -v            Display internal status messages.\n");
    fprintf(stderr, "  -D            Display internal debug messages.\n");
}

int
main(int argc, char **argv)
{
    int badopt, rc;
    int opt_prefix, opt_status, opt_quiet, opt_internal, opt_debug;
    int opt_ctimeout, opt_outmode, opt_maxworkers, opt_vtest;
    u_int opt_test, opt_analyzer;
    char *opt_analyze, *opt_outanalysis, *opt_erranalysis;
    char *opt_spawn, *opt_command, *opt_odir, *opt_ping, *opt_rcmd;
    char tdir[PATH_MAX];
    int longest;
    time_t start;

    myname = basename(argv[0]);

    opt_prefix = opt_status = 1;
    opt_quiet = opt_internal = opt_debug = 0;
    opt_outmode = OUT_MIXED;
    opt_maxworkers = DEFAULT_MAXWORKERS;
    opt_ctimeout = opt_test = opt_vtest = 0;
    opt_analyze = opt_outanalysis = opt_erranalysis = NULL;
    opt_command = opt_odir = opt_ping = NULL;
    opt_rcmd = getenv("SHMUX_RCMD");
    opt_spawn = NULL;
    if (getenv("SHMUX_SPAWN") != NULL)
	opt_spawn = getenv("SHMUX_SPAWN");
    if (opt_rcmd == NULL)
	opt_rcmd = DEFAULT_RCMD;
    if (getenv("SHMUX_ERRORCODES") != NULL)
	byteset_init(BSET_ERROR, getenv("SHMUX_ERRORCODES"));
    else
	byteset_init(BSET_ERROR, DEFAULT_ERRORCODES);
    if (getenv("SHMUX_SHOWCODES") != NULL)
	byteset_init(BSET_SHOW, getenv("SHMUX_SHOWCODES"));
    else
	byteset_init(BSET_SHOW, "");

    badopt = 0;
    while (1)
      {
        int c;
	
        c = getopt(argc, argv, "a:A:bc:C:De:E:hmM:o:pP:qQr:sS:tT:vV");
	
        /* Detect the end of the options. */
        if (c == -1)
            break;
	
        switch (c)
          {
	  case 'a':
	      opt_analyze = optarg;
	      break;
	  case 'A':
	      if (opt_outanalysis == NULL)
		  opt_outanalysis = optarg;
	      else
		  opt_erranalysis = optarg;
	      break;
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
	  case 'e':
	      byteset_init(BSET_ERROR, optarg);
	      break;
	  case 'E':
	      byteset_init(BSET_SHOW, optarg);
	      break;
          case 'h':
              usage(1);
              exit(RC_OK);
              break;
	  case 'm':
	      opt_outmode &= ~(OUT_NULL|OUT_MIXED);
	      opt_outmode |= OUT_ATEND;
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
	      if ((opt_outmode & OUT_IFERR) == 0)
		  opt_outmode |= OUT_IFERR;
	      else
		{
		  opt_outmode = OUT_NULL;
		  if (opt_spawn == NULL)
		      opt_spawn = "all";
		}
	      break;
	  case 'Q':
	      opt_quiet = 1;
	      break;
	  case 'r':
	      opt_rcmd = optarg;
	      break;
	  case 's':
	      opt_status = 0;
	      break;
	  case 'S':
	      opt_spawn = optarg;
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
#if !defined(HAVE_PCRE_H)
	      printf("%s version %s\n", myname, SHMUX_VERSION);
#else
	      printf("%s version %s (PCRE version %s)\n",
		     myname, SHMUX_VERSION, pcre_version());
#endif
	      exit(RC_OK);
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
        exit(RC_ERROR);
      }

    target_default(opt_rcmd);
    if (opt_maxworkers <= 0)
      {
	fprintf(stderr, "%s: Invalid -M argument!\n", myname);
	exit(RC_ERROR);
      }

    if (opt_spawn == NULL)
	opt_spawn = DEFAULT_SPAWNMODE;

    opt_analyzer = analyzer_init(opt_analyze, opt_outanalysis,opt_erranalysis);
    /* -A requires -o, to avoid dangerous/reckless invocations. */
    if (opt_analyzer != ANALYZE_NONE && opt_odir == NULL)
      {
	fprintf(stderr, "%s: -o option required when using -a/-A!\n", myname);
	exit(RC_ERROR);
      }

    /* -? requires -o, to avoid dangerous/reckless invocations. */
    if ((opt_outmode & (OUT_NULL|OUT_IFERR)) != 0 && opt_odir == NULL)
      {
	fprintf(stderr, "%s: -o option required when using -q!\n", myname);
	exit(RC_ERROR);
      }

    if (opt_odir != NULL)
	opt_outmode |= OUT_COPY;
    else if ((opt_outmode & OUT_ATEND) != 0)
      {
	/* We'll need a temporary directory */
	char *tmp;

	tmp = getenv("TMPDIR");
	sprintf(tdir, "%s/%s.%d.%ld", (tmp != NULL) ? tmp : _PATH_TMP,
		myname, (int) getpid(), (long) time(NULL));
	opt_odir = tdir;
      }

    if (opt_odir != NULL && mkdir(opt_odir, 0777) == -1 && errno != EEXIST)
      {
	/* Create odir if it doesn't already exists */
	fprintf(stderr, "%s: mkdir(%s): %s\n",
		myname, opt_odir, strerror(errno));
	exit(RC_ERROR);
      }

    /* Should test outputs be shown? */
    if (opt_vtest > 1)
	opt_test *= -1;

    /* Get list of targets */
    longest = strlen(myname);
    while (optind < argc)
      {
	int length;

	if (strcmp(argv[optind], "-") != 0)
	    length = target_add(argv[optind++]);
	else
	  {
	    char tname[256];

	    optind += 1;
	    while (fgets(tname, 256, stdin) != NULL)
	      {
		tname[strlen(tname)-1] = '\0';
		length = target_add(tname);
	      }
	  }
	if (length > longest)
	    longest = length;
      }

    /* Initialize terminal */
    term_init(longest, opt_prefix, opt_status, opt_internal, opt_debug);

    /* Loop through targets/commands */
    start = time(NULL);
    rc = loop(opt_command, opt_ctimeout, opt_maxworkers, opt_spawn,
	      opt_outmode, opt_odir, opt_analyzer, opt_ping, opt_test);

    /* Summary of results unless asked to be quiet */
    if (opt_quiet == 0)
      {
	nprint("");
	target_results((int) (time(NULL) - start));
      }

    /* odir was temporary, remove it now */
    if (opt_odir != NULL && (opt_outmode & OUT_COPY) == 0
	&& rmdir(opt_odir) == -1)
	fprintf(stderr, "%s: rmdir(%s): %s\n",
		myname, opt_odir, strerror(errno));

    exit(rc);
}
