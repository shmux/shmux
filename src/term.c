/*
** Copyright (C) 2002, 2003 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#if defined(HAVE_TERMCAP_H)
# include <termcap.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
# if defined(HAVE_TERM_H) && defined(sun)
#  include <term.h>
# endif
#endif
#include <termios.h>
#include <signal.h>
#include <stdarg.h>

#include "term.h"

static char const rcsid[] = "@(#)$Id: term.c,v 1.17 2003-04-26 01:32:17 kalt Exp $";

extern char *myname;

static int targets, internalmsgs, debugmsgs, padding;
static struct termios origt, shmuxt;
static int otty, etty, CO, got_sigwin, ttyin;
static char *MD,			/* bold */
	    *ME,			/* turn off bold (and more) */
	    *LE,			/* move cursor left one position */
	    *CE,			/* clear to end of line */
	    *CR,			/* carriage return */
	    *NL;			/* newline character if not \n */
static char status[512];
static FILE *ttyout;

static void shmux_signal(int);
static void tty_init(void);
static int putchar2(int);
static int putchar3(int);
static void gprint(char *, char, char *, va_list);

/*
** shmux_signal
**	signal handler
*/
static void
shmux_signal(sig)
int sig;
{
    if (sig == SIGWINCH || sig == SIGCONT)
	got_sigwin += 1;
    if (sig == SIGCONT)
      {
	if (ttyin >= 0 && tcsetattr(ttyin, TCSANOW, &shmuxt) < 0)
	    /* Calling eprint from here isn't so smart.. */
	    eprint("tcsetattr() failed: %s", strerror(errno));
      }
    if (sig == SIGINT || sig == SIGQUIT || sig == SIGABRT || sig == SIGTERM
	|| sig == SIGTSTP)
      {
	/* restore original tty settings */
	if (ttyin >= 0 && tcsetattr(ttyin, TCSANOW, &origt) < 0)
	    /* Calling eprint from here isn't so smart.. */
	    eprint("tcsetattr() failed: %s", strerror(errno));
	if (sig != SIGTSTP)
	    /* Resend the signal to ourselves */
	    kill(getpid(), sig);
	else
	    kill(getpid(), SIGSTOP);
      }
}

/*
** term_init:
**	Initialize terminal handling system.
*/
void
term_init(maxlen, prefix, progress, internal, debug)
int maxlen, prefix, progress, internal, debug;
{
    static char termcap[2048], area[1024];
    char *term, *ptr;
    struct sigaction sa;

    assert( maxlen != 0 );

    padding = prefix*maxlen;
    targets = prefix;
    internalmsgs = internal;
    debugmsgs = debug;

    /* Input initialization */

    if (isatty(fileno(stdin)) == 1)
	ttyin = fileno(stdin);
    else
	ttyin = open("/dev/tty", O_RDONLY, 0);

    /* Output initializations */

    LE = NULL;
    CE = NULL;
    CR = "\r";
    NL = "\n";
    status[0] = '\0';

    otty = isatty(fileno(stdout));
    etty = isatty(fileno(stderr));

    if (otty == 1)
	ttyout = stdout;
    else if (etty == 1)
	ttyout = stderr;
    else if (progress != 0)
	ttyout = fopen("/dev/tty", "a");
    else
	ttyout = NULL;

    term = getenv("TERM");

    if (debugmsgs != 0)
	fprintf(stdout,
	     "%*s$ itty[%d] ttyin[%d] otty[%d] etty[%d] ttyout[%p] TERM[%s]\n",
		padding, myname, isatty(fileno(stdin)), ttyin,
		otty, etty, ttyout, (term != NULL) ? term : "");

    if (term == NULL)
      {
	if (progress != 0 && ttyout != NULL)
	    fprintf(stderr, "%*s: TERM variable is not set!\n",
		    padding, myname);
	tty_init();
	return;
      }
    if (tgetent(termcap, term) < 1)
      {
	if (progress != 0 && ttyout != NULL)
	    fprintf(stderr, "%*s: No TERMCAP entry for ``%s''.\n",
		    padding, myname, term);
	tty_init();
	return;
      }

    ptr = area;

    /* Get terminal width and setup signal handler to keep track of it. */
    if ((CO = tgetnum("co")) == -1) CO = 80;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = shmux_signal;
    sigaction(SIGWINCH, &sa, NULL);
    sigaction(SIGCONT, &sa, NULL);
    got_sigwin = 0;
    term_size();

    MD = tgetstr("md", &ptr);
    if (MD != NULL)
      {
	ME = tgetstr("me", &ptr);
	if (ME == NULL)
	    MD = NULL;
      }
    else
	ME = NULL;
    CR = tgetstr("cr", &ptr);
    if (CR == NULL)
	CR = "\r"; /* Let's just assume.. */
    NL = tgetstr("nl", &ptr);
    if (NL == NULL)
	NL = "\n"; /* As per specification. */
    LE = tgetstr("le", &ptr);
    CE = tgetstr("ce", &ptr);
    if (progress == 0)
	CE = NULL;
    else if (CE == NULL && progress != 0 && ttyout != NULL)
	fprintf(stderr, "%*s: Terminal ``%s'' is too dumb! (no ce)\n",
		padding, myname, term);

    tty_init();
}

/*
** term_size:
**	Query /dev/tty to find its size
*/
void
term_size(void)
{
#if defined(TIOCGWINSZ)
  struct winsize ws;

  if (ttyout == NULL)
      return;
  if (ioctl(fileno(ttyout), TIOCGWINSZ, &ws) < 0)
    {
      eprint("ioctl(tty, TIOCGWINSZ) failed: %s", strerror(errno));    
      return;
    }
  if (ws.ws_col > 0)
    {
      CO = ws.ws_col;
      dprint("window seems to have %d columns.", CO);
    }
#endif
}

/*
** tty_init
**	/dev/tty initialization
*/
static void
tty_init(void)
{
    if (ttyin < 0)
      {
	dprint("No input tty available.");
	return;
      }

    if (tcgetattr(ttyin, &origt) < 0) /* save original tty settings */
      {
	eprint("tcgetattr() failed: %s", strerror(errno));
	ttyin = -1;
      }
    else
      {
	shmuxt = origt;
	shmuxt.c_lflag &= ~(ICANON|ECHO); /* no echo or canonical processing */
	shmuxt.c_cc[VMIN] = 1; /* no buffering */
	shmuxt.c_cc[VTIME] = 0; /* no delaying */
	if (tcsetattr(ttyin, TCSANOW, &shmuxt) == 0)
	  {
	    struct sigaction sa;

	    atexit(tty_restore);

	    /*
	    ** Catch signals which require reinitializing or restoring
	    ** tty settings
	    */
	    sigemptyset(&sa.sa_mask);
	    sa.sa_flags = 0;
	    sa.sa_handler = shmux_signal;
	    sigaction(SIGTSTP, &sa, NULL);
	    /* Only need to catch the following signals once, then we die. */
	    sa.sa_flags = SA_RESETHAND;
	    sigaction(SIGINT, &sa, NULL);
	    sigaction(SIGQUIT, &sa, NULL);
	    sigaction(SIGABRT, &sa, NULL);
	    sigaction(SIGTERM, &sa, NULL);

	    dprint("Input tty initialized");
	  }
	else
	  {
	    eprint("tcsetattr() failed: %s", strerror(errno));
	    ttyin = -1;
	  }
      }
}

/*
** tty_fd
**	Returns the file descriptor associated with the tty input
*/
int
tty_fd(void)
{
    return ttyin;
}

/*
** tty_restore
**	Restores /dev/tty original settings
*/
void
tty_restore(void)
{
    /* restore original tty settings */
    if (ttyin >= 0 && tcsetattr(ttyin, TCSANOW, &origt) < 0)
	eprint("tcsetattr() failed: %s", strerror(errno));
    ttyin = -1;
}

/*
** sprint:
**	progress status printf wrapper responsible for displaying a string
**	on a pseudo status line, leaving the cursor at the beginning of the
**	line so that it can easily be overwritten afterwards.
*/
void
sprint(char *format, ...)
{
    char *ch;
    int bold;

    if (CE == NULL || ttyout == NULL)
	return;

    if (got_sigwin > 0)
      {
	got_sigwin = 0;
	term_size();
      }

    if (format != NULL)
      {
        va_list va;
	char tmp[512];

        va_start(va, format);
        vsnprintf(tmp, 512, format, va);
        va_end(va);
	tmp[CO-1] = '\0';
	if (strcmp(tmp, status) == 0)
	    return;
	strcpy(status, tmp);
      }

    bold = 0;
    ch = status;
    while (*ch != '\0')
      {
	if (*ch == '\a' && bold == 0)
	  {
	    if (otty == 1 && MD != NULL)
	      {
		tputs(MD, 0, putchar3);
		bold = 1;
	      }
	  }
	else
	  {
	    if ((*ch == ' ' || *ch == '\a') && bold == 1)
	      {
		tputs(ME, 0, putchar3);
		bold = 0;
	      }
	    fprintf(ttyout, "%c", *ch);
	  }
	ch += 1;
      }

    tputs(CE, 0, putchar3);
    tputs(CR, 0, putchar3);
    fflush(ttyout);
}

/*
** putchar2:
**	Same as putchar, for stderr.
*/
static int
putchar2(c)
int c;
{
    return fputc(c, stderr);
}

/*
** putchar3:
**	Same as putchar, for tty (either stdout or stderr).
*/
static int
putchar3(c)
int c;
{
    return fputc(c, ttyout);
}

/*
** uprint:
**	printf wrapper for messages destined to the user (e.g. /dev/tty)
*/
void
uprint(char *format, ...)
{
    va_list va;

    if (CE != NULL)
	tputs(CE, 0, putchar3);

    fprintf(ttyout, ">> ");
    va_start(va, format);
    vfprintf(ttyout, format, va);
    va_end(va);
    tputs(NL, 0, putchar3);
    fflush(ttyout);

    if (CE != NULL)
	sprint(NULL);
}

/*
** uprompt:
**	prompt the user (e.g. /dev/tty)
*/
char *
uprompt(char *format, ...)
{
    va_list va;
    static char input[1024];
    int pos;

    if (CE != NULL)
	tputs(CE, 0, putchar3);

    /* Display the prompt. */
    fprintf(ttyout, ">> ");
    va_start(va, format);
    vfprintf(ttyout, format, va);
    va_end(va);
    fprintf(ttyout, " ");
    fflush(ttyout);

    /* Read/process user input */
    pos = -1;
    while (1)
      {
	switch (read(ttyin, input+pos+1, 1))
	  {
	  case -1:
	      eprint("Unexpected read(/dev/tty) error: %s", strerror(errno));
	      pos = -2;
	      break;
	  case 0:
	      eprint("Unexpected empty read(/dev/tty) result");
	      pos = -2;
	      break;
	  default:
	      pos += 1;
	      break;
	  }
	if (pos < 0)
	    break;

	if (input[pos] == origt.c_cc[VERASE])
	  {
	    pos -= 1;
	    if (pos >= 0)
	      {
		pos -= 1;
		tputs(LE, 0, putchar3);
		tputs(CE, 0, putchar3);
	      }
	  }
	else if (input[pos] == origt.c_cc[VKILL])
	  {
	    pos -= 1;
	    while (pos >= 0)
	      {
		pos -= 1;
		tputs(LE, 0, putchar3);
	      }
	    tputs(CE, 0, putchar3);
	  }
	else if (input[pos] == '\n')
	  {
	    input[pos] = '\0';
	    break;
	  }
	else
	  {
	    if (pos < 1023)
	      {
		fprintf(ttyout, "%c", input[pos]);
		tputs(CE, 0, putchar3); /* Useful after a multiline VKILL */
	      }
	    else
		pos -= 1;
	  }
	fflush(ttyout);
      }

    tputs(NL, 0, putchar3);
    fflush(ttyout);

    if (CE != NULL)
	sprint(NULL);

    if (pos == -2)
	return NULL;
    input[pos+1] = '\0';
    return input;
}

/*
** gprint:
**	generic (private) printf wrapper used by (most of) the public wrappers
**	to display a line with the proper indentation.
*/
static void
gprint(char *prefix, char separator, char *format, va_list va)
{
    FILE *std;
    int (*pc)(int);

    std = stdout; pc = putchar;
    if (separator == MSG_STDERR || separator == MSG_STDERRTRUNC)
      {
	std = stderr; pc = putchar2;
	if (etty == 1 && MD != NULL)
	    tputs(MD, 0, pc);
      }

    if (CE != NULL)
	tputs(CE, 0, pc);

    if ((prefix != NULL && targets != 0) || (prefix == myname))
	fprintf(std, "%*s%c ", padding, prefix, separator);
    vfprintf(std, format, va);
    if (std == stderr && etty == 1 && ME != NULL)
	tputs(ME, 0, pc);
    tputs(NL, 0, pc);
    fflush(std);

    if (CE != NULL)
	sprint(NULL);
}

/*
** dprint:
**	debug printf wrapper
*/
void
dprint(char *format, ...)
{
    va_list va;

    if (debugmsgs == 0)
	return;

    va_start(va, format);
    gprint(myname, MSG_DEBUG, format, va);
    va_end(va);
}

/*
** iprint:
**	internal printf wrapper
*/
void
iprint(char *format, ...)
{
    va_list va;

    if (internalmsgs == 0)
	return;

    va_start(va, format);
    gprint(myname, MSG_STDOUT, format, va);
    va_end(va);
}

/*
** eprint:
**	internal printf wrapper for errors
*/
void
eprint(char *format, ...)
{
    va_list va;

    va_start(va, format);
    gprint(myname, MSG_STDERR, format, va);
    va_end(va);
}

/*
** tprint:
**	general printf wrapper
*/
void
tprint(char *target, char separator, char *format, ...)
{
    va_list va;

    va_start(va, format);
    gprint(target, separator, format, va);
    va_end(va);
}

/*
** nprint:
**	normal printf wrapper.. total overkill
*/
void
nprint(char *format, ...)
{
    va_list va;

    va_start(va, format);
    vprintf(format, va);
    va_end(va);
    tputs(NL, 0, putchar);
}
