/*
** Copyright (C) 2002 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#if defined(HAVE_TERMCAP_H)
# include <termcap.h>
#endif
#if defined(HAVE_CURSES_H)
# include <curses.h>
# if defined(HAVE_TERM_H)
#  include <term.h>
# endif
#endif
#include <stdarg.h>

#include "term.h"

static char const rcsid[] = "@(#)$Id: term.c,v 1.6 2002-10-13 21:12:45 kalt Exp $";

extern char *myname;

static int targets, internalmsgs, debugmsgs, padding;
static int otty, etty;
static char *MD,			/* bold */
	    *ME,			/* turn off bold (and more) */
	    *CE,			/* clear to end of line */
	    *CR,			/* carriage return */
	    *NL;			/* newline character if not \n */
static char status[512];

static int putchar2(int);
static void gprint(char *, char, char *, va_list);

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

    assert( maxlen != 0 );

    padding = prefix*maxlen;
    targets = prefix;
    internalmsgs = internal;
    debugmsgs = debug;

    CE = NULL;
    CR = "\r";
    NL = "\n";
    status[0] = '\0';

    term = getenv("TERM");
    if (term == NULL)
      {
	if (progress != 0)
	    fprintf(stderr, "%*s: TERM variable is not set!\n",
		    padding, myname);
	return;
      }
    if (tgetent(termcap, term) < 1)
      {
	if (progress != 0)
	    fprintf(stderr, "%*s: No TERMCAP entry for ``%s''.\n",
		    padding, myname, term);
	return;
      }

    ptr = area;

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
    CE = tgetstr("ce", &ptr);
    if (progress == 0)
	CE = NULL;
    else if (CE == NULL)
	fprintf(stderr, "%*s: Terminal ``%s'' is too dumb! (no ce)\n",
		padding, myname, term);

    otty = isatty(1);
    etty = isatty(2);
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

    if (CE == NULL || otty == 0)
	return;

    if (format != NULL)
      {
        va_list va;
	char tmp[512];

        va_start(va, format);
        vsnprintf(tmp, 512, format, va);
        va_end(va);
	if (strcmp(tmp, status) == 0)
	    return;
	strcpy(status, tmp);
      }

    bold = 0;
    ch = status;
    while (*ch != '\0')
      {
	if (*ch == '\a')
	  {
	    if (otty == 1 && MD != NULL)
	      {
		tputs(MD, 0, putchar);
		bold = 1;
	      }
	  }
	else
	  {
	    if (*ch == ' ' && bold == 1)
	      {
		tputs(ME, 0, putchar);
		bold = 0;
	      }
	    printf("%c", *ch);
	  }
	ch += 1;
      }

    tputs(CE, 0, putchar);
    tputs(CR, 0, putchar);
    fflush(stdout);
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
    tputs(NL, 0, putchar);
    va_end(va);
}
