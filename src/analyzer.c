/*
** Copyright (C) 2003 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
*/

#include "os.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <regex.h>
#if defined(HAVE_PCRE_H)
# include <pcre.h>
#endif

#include "analyzer.h"
#include "target.h"
#include "term.h"
#include "units.h"

static char const rcsid[] = "@(#)$Id: analyzer.c,v 1.2 2003-03-23 18:34:54 kalt Exp $";

extern char *myname;

static int     out_ok, err_ok;
static regex_t out_re, err_re;
#if defined(HAVE_PCRE_H)
static pcre *out_pcre, *err_pcre;
#endif
static char    *run_cmd;
static u_int	run_timeout;

static void *mapfile(int, int, char *, size_t *);
static void unmapfile(int, char *, void *, size_t);
static void re_comp(void *, char *);
static void pcre_comp(void *, char *);
static void restr_init(void *, void (*)(void *, char *), int *, char *);

/*
** mapfile
**	Given an open file descriptor, mmap() the corresponding (regular) file
*/
static void *
mapfile(errto, fd, name, len)
int errto, fd;
char *name;
size_t *len;
{
    struct stat sb;
    void *mm;

    if (fstat(fd, &sb) == -1)
      {
	/* Can this fail at all? */
	if (errto == 1)
	    fprintf(stderr, "%s: fstat(%d [%s]) failed: %s\n",
		    myname, fd, name, strerror(errno));
	else
	    eprint("Fatal internal error for %s output analysis: fstat(%d [%s]) failed: %s", target_getname(), fd, name, strerror(errno));
	return NULL;
      }
    *len = sb.st_size;

    if (*len == 0)
	return "";
    
    mm = mmap(NULL, *len, PROT_READ, MAP_PRIVATE
#if defined(MAP_FILE)
	       |MAP_FILE
#endif
#if defined(MAP_COPY)
	       |MAP_COPY
#endif
	       , fd, 0);

    if (mm == MAP_FAILED)
      {
	/* Can this fail at all? */
	if (errto == 1)
	    fprintf(stderr, "%s: mmap(%d [%s]) failed: %s\n",
		    myname, fd, name, strerror(errno));
	else
	    eprint("Fatal internal error for %s output analysis: mmap(%d [%s]) failed: %s", target_getname(), fd, name, strerror(errno));
	return NULL;
      }

    return mm;
}

/*
** unmapfile
**	munmap() a previously mapped file
*/
static void
unmapfile(errto, name, mm, len)
int errto;
char *name;
void *mm;
size_t len;
{
    if (len == 0)
	return;

    if (munmap(mm, len) == -1)
      {
	if (errto == 1)
	  {
	    fprintf(stderr, "%s: munmap(%s) failed: %s\n",
		    myname, name, strerror(errno));
	    exit(1);
	  }
	else
	    eprint("munmap(%s) failed: %s", name, strerror(errno));
      }
}

/*
** re_comp
**	Regular expression (regex_t) initialization
*/
void
re_comp(reptr, str)
void *reptr;
char *str;
{
    regex_t *re;
    int errcode;

    re = reptr;
    errcode = regcomp(re, str, REG_EXTENDED|REG_NOSUB|REG_NEWLINE);
    if (errcode != 0)
      {
	char buf[1024];
	
	if (regerror(errcode, re, buf, 1024) != 0)
	    fprintf(stderr, "%s: Bad regular expression: %s\n", myname, buf);
	else
	    fprintf(stderr, "%s: Failed to compile regular expression and to obtain error code: %s\n", myname, str);
	exit(1);
      }
}

/*
** pcre_comp
**	Perl Compatible Regular Expression (pcre) initialization
*/
void
pcre_comp(pcreptr, str)
void *pcreptr;
char *str;
{
    pcre **re;
    const char *error;
    int erroffset;

    re = pcreptr;
    *re = pcre_compile(str, PCRE_MULTILINE, &error, &erroffset, NULL);
    if (*re == NULL)
      {
	fprintf(stderr, "%s: Bad PCRE (offset %d): %s\n",
		myname, erroffset, error);
	exit(1);
      }
}

/*
** re_init
**	Regular expression initialization routine
*/
static void
restr_init(re, comp, ok, str)
void *re;
void (*comp)(void *, char *);
int *ok;
char *str;
{
    char *fname;
    int fd;
    size_t len;

    fname = NULL;
    fd = -1;
    len = 0;
    *ok = 0;
    if (str[0] == '=' || str[0] == '!')
      {
	if (str[0] == '!')
	    *ok = 1;
	str += 1;
	if (str[0] == '=')
	    str += 1; /* expression follows */
	else if (str[0] == '<')
	  {
	    /* Expression is in a file */
	    fname = str+1;
	    fd = open(fname, O_RDONLY, 0);
	    if (fd == -1)
	      {
		fprintf(stderr, "%s: open(%s) failed: %s\n",
			myname, fname, strerror(errno));
		exit(1);
	      }
	    str = mapfile(1, fd, fname, &len);
	    if (str == NULL)
		exit(1);
	  }
      }

    comp(re, str);

    if (fname != NULL)
	unmapfile(1, fname, str, len);
    if (fd >= 0)
	close(fd);
}

/*
** analyzer_init
**	Initialization for the analyzer, called early on.
*/
int
analyzer_init(type, out, err)
char *type, *out, *err;
{
    if (type == NULL)
	return ANALYZE_NONE;

    if (out == NULL)
      {
	fprintf(stderr, "%s: No analysis defined!\n", myname); /* XXX */
	exit(1);
      }
	
    if (strcmp(type, "run") == 0)
      {
	run_cmd = out;
	if (err == NULL)
	    run_timeout = 15;
	else
	    run_timeout = unit_time(err);
	return ANALYZE_RUN;
      }
    else if (strcmp(type, "regex") == 0 || strcmp(type, "re") == 0
	     || strcmp(type, "pcre") == 0)
      {
	if (type[0] != 'p')
	    restr_init((void *) &out_re, re_comp, &out_ok, out);
#if defined(HAVE_PCRE_H)
	else
	    restr_init((void *) &out_pcre, pcre_comp, &out_ok, out);
#endif

	if (err == NULL)
	    err = "!."; /* Impose sane/useful default */

	if (type[0] != 'p')
	    restr_init((void *) &err_re, re_comp, &err_ok, err);
#if defined(HAVE_PCRE_H)
	else
	    restr_init((void *) &err_pcre, pcre_comp, &err_ok, err);
#endif

	if (type[0] != 'p')
	    return ANALYZE_RE;
	else
	    return ANALYZE_PCRE;
      }
    else
      {
	fprintf(stderr, "%s: Invalid -a argument!\n", myname);
	exit(1);
      }
}

/*
** analyzer_run
**	Analyze output from a target according to user specified regular
**	expressions.
*/
int
analyzer_run(type, ofd, oname, efd, ename)
u_int type;
int ofd, efd;
char *oname, *ename;
{
    size_t olen, elen;
    void *out, *err;
    int o, e, ok;

    assert( type != ANALYZE_RUN );

    if (oname == NULL || ename == NULL || ofd == -1 || efd == -1)
      {
	eprint("Unable to analyze output for %s!  (Missing output)",
	       target_getname());
	return -1;
      }


    out = mapfile(2, ofd, oname, &olen);
    if (out == NULL)
	return -1;
    err = mapfile(2, efd, ename, &elen);
    if (err == NULL)
      {
	unmapfile(2, oname, out, olen);
	return -1;
      }
    
    ok = 0;
    if (type == ANALYZE_RE)
      {
	o = regexec(&out_re, (char *) out, 0, NULL, 0);
	if (o != 0 && o != REG_NOMATCH)
	  {
	    char buf[1024];
	    
	    if (regerror(o, &out_re, buf, 1024) != 0)
		eprint("Fatal error for %s output analysis: regexec() failed with code %s", target_getname(), buf);
	    else
		eprint("Fatal error for %s output analysis: regexec() failed with code %d", target_getname(), o);
	    ok = -1;
	  }
	else if ((o == 0 && out_ok != 0)
		 || (o == REG_NOMATCH && out_ok == 0))
	  {
	    ok = 1;
	    dprint("Analysis for %s: out=%d[%d] err=?[%d] ok=%d (REG_NOMATCH=%d)", target_getname(), o, out_ok, err_ok, ok, REG_NOMATCH);
	  }
	else
	  {
	    e = regexec(&err_re, (char *) err, 0, NULL, 0);
	    if (e != 0 && e != REG_NOMATCH)
	      {
		char buf[1024];
		
		if (regerror(e, &err_re, buf, 1024) != 0)
		    eprint("Fatal error for %s output analysis: regexec() failed with code %s", target_getname(), buf);
		else
		    eprint("Fatal error for %s output analysis: regexec() failed with code %d", target_getname(), o);
		ok = -1;
	      }
	    else if ((e == 0 && err_ok != 0)
		     || (e == REG_NOMATCH && err_ok == 0))
		ok = 1;
	    
	    dprint("Analysis for %s: out=%d[%d] err=%d[%d] ok=%d (REG_NOMATCH=%d)", target_getname(), o, out_ok, e, err_ok, ok, REG_NOMATCH);
	  }
      }
#if defined(HAVE_PCRE_H)
    else
      {
	o = pcre_exec(out_pcre, NULL, (char *) out, olen, 0, 0, NULL, 0);
	if (o < 0 && o != PCRE_ERROR_NOMATCH)
	  {
	    eprint("Fatal error for %s output analysis: pcre_exec() failed with code %d", target_getname(), o);
	    ok = -1;
	  }
	else if ((o >= 0 && out_ok != 0)
		 || (o == PCRE_ERROR_NOMATCH && out_ok == 0))
	  {
	    ok = 1;
	    dprint("Analysis for %s: out=%d[%d] err=?[%d] ok=%d (PCRE_ERROR_NOMATCH=%d)", target_getname(), o, out_ok, err_ok, ok, PCRE_ERROR_NOMATCH);
	  }
	else
	  {
	    e = pcre_exec(err_pcre, NULL, (char *) err, elen, 0, 0, NULL, 0);
	    if (e < 0 && e != PCRE_ERROR_NOMATCH)
	      {
		eprint("Fatal error for %s output analysis: pcre_exec() failed with code %d", target_getname(), e);
		ok = -1;
	      }
	    else if ((e >= 0 && err_ok != 0)
		     || (e == PCRE_ERROR_NOMATCH && err_ok == 0))
		ok = 1;
	    
	    dprint("Analysis for %s: out=%d[%d] err=%d[%d] ok=%d (PCRE_ERROR_NOMATCH=%d)", target_getname(), o, out_ok, e, err_ok, ok, PCRE_ERROR_NOMATCH);
	  }
      }
#endif

    unmapfile(2, oname, out, olen);
    unmapfile(2, ename, err, elen);
    
    return ok;
}

/*
** analyzer_cmd
**	Returns the external analyzer command to run
*/
char *
analyzer_cmd(void)
{
    return run_cmd;
}

/*
** analyzer_timeout
**	Returns the timeout for the external analyzer
*/
u_int
analyzer_timeout(void)
{
    return run_timeout;
}
