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

static char const rcsid[] = "@(#)$Id: analyzer.c,v 1.7 2003-05-07 00:57:07 kalt Exp $";

extern char *myname;

struct condition
{
    int ok;
    union
    {
	regex_t re;
#if defined(HAVE_PCRE_H)
	pcre *pcre;
#endif
    } val;
};

static struct condition *out, *err;

static char    *run_cmd;
static u_int	run_timeout;

static void *mapfile(int, int, char *, size_t *);
static void unmapfile(int, char *, void *, size_t);
static void compile_re(int, void *, char *);
#if defined(HAVE_PCRE_H)
static void compile_pcre(int, void *, char *);
#endif
static void restr_init(void *, void (*)(int, void *, char *), int *, char *);
static void loadfile(int, char *, struct condition **list);

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
    
    mm = mmap(NULL, *len + 1, PROT_READ|PROT_WRITE, MAP_PRIVATE
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

    ((char *) mm)[*len] = '\0';

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
** compile_re
**	Regular expression (regex_t) initialization
*/
void
compile_re(mline, reptr, str)
int mline;
void *reptr;
char *str;
{
    regex_t *re;
    int errcode;

    re = reptr;
    errcode = regcomp(re, str,
		      (mline != 0) ? REG_EXTENDED|REG_NOSUB|REG_NEWLINE
		      : REG_EXTENDED|REG_NOSUB);
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

#if defined(HAVE_PCRE_H)
/*
** compile_pcre
**	Perl Compatible Regular Expression (pcre) initialization
*/
void
compile_pcre(mline, pcreptr, str)
int mline;
void *pcreptr;
char *str;
{
    pcre **re;
    const char *error;
    int erroffset;

    re = pcreptr;
    *re = pcre_compile(str, (mline != 0) ? PCRE_MULTILINE : 0,
		       &error, &erroffset, NULL);
    if (*re == NULL)
      {
	fprintf(stderr, "%s: Bad PCRE (offset %d): %s\n",
		myname, erroffset, error);
	exit(1);
      }
}
#endif

/*
** re_init
**	Regular expression initialization routine
*/
static void
restr_init(re, comp, ok, str)
void *re;
void (*comp)(int, void *, char *);
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

    comp(1, re, str);

    if (fname != NULL)
	unmapfile(1, fname, str, len);
    if (fd >= 0)
	close(fd);
}

/*
** loadfile
**	Read a list of one line conditions from a file
*/
static void
loadfile(type, file, list)
int type;
char *file;
struct condition **list;
{
    int fd, lineno, cond, max;
    size_t len;
    char *str, *ln, *nl;

    assert( *list == NULL );

    /* Allocate a list of conditions */
    max = 10;
    *list = (struct condition *) malloc(max * sizeof(struct condition));
    if (*list == NULL)
      {
	fprintf(stderr, "%s: malloc() failed: %s", myname, strerror(errno));
	exit(1);
      }
    cond = 0;

    fd = -1;
    len = 0;
    fd = open(file, O_RDONLY, 0);
    if (fd == -1)
      {
	fprintf(stderr, "%s: open(%s) failed: %s\n", 
		myname, file, strerror(errno));
	exit(1);
      }
    str = mapfile(1, fd, file, &len);
    if (str == NULL)
	exit(1);

    ln = str; nl = str;
    lineno = 1;
    while (nl - str < len)
    {
      /* Parse the file line by line */
      while (nl - str < len && *nl != '\n')
	  nl += 1;
      assert( *nl == '\0' || *nl == '\n' );
      *nl = '\0';

      /* Make sure the list structure is big enough */
      if (cond == max-2)
	{
	  max *= 2;
	  *list = (struct condition *) realloc(*list,
					       max* sizeof(struct condition *));
	  if (*list == NULL)
	    {
	      fprintf(stderr, "%s: realloc() failed: %s", myname,
		      strerror(errno));
	      exit(1);
	    }
	}

      /* Finally, compile the expression */
      if (*ln == '=' || *ln == '~' || *ln == '!')
	{
	  if (*ln != '!')
	      (*list)[cond].ok = 0;
	  else
	      (*list)[cond].ok = 1;
	  if (type == ANALYZE_LNRE)
	      compile_re(0, (void *) &((*list)[cond].val.re), ln + 1);
#if defined(HAVE_PCRE_H)
	  else if (type == ANALYZE_LNPCRE)
	      compile_pcre(0, (void *) &((*list)[cond].val.pcre), ln + 1);
#endif
	  else
	      abort();
	  cond += 1;
	}
      else if (*ln != '\0')
	{
	  fprintf(stderr, "Invalid configuration in \"%s\" line %d: %s\n",
		  file, lineno, ln);
	  exit(1);
	}
      ln = nl + 1;
      lineno += 1;
    }

    (*list)[cond].ok = -1; /* Mark the end of the list */

    if (file != NULL)
	unmapfile(1, file, str, len);
    if (fd >= 0)
	close(fd);
}

/*
** analyzer_init
**	Initialization for the analyzer, called early on.
*/
int
analyzer_init(type, outdef, errdef)
char *type, *outdef, *errdef;
{
    if (type == NULL)
	return ANALYZE_NONE;

    if (outdef == NULL)
      {
	fprintf(stderr, "%s: No analysis criteria defined!\n", myname);
	exit(1);
      }
	
    if (strcmp(type, "run") == 0)
      {
	run_cmd = outdef;
	if (errdef == NULL)
	    run_timeout = 15;
	else
	    run_timeout = unit_time(errdef);
	return ANALYZE_RUN;
      }
    else if (strcmp(type, "regex") == 0 || strcmp(type, "re") == 0
	     || strcmp(type, "pcre") == 0)
      {
	out = (struct condition *) malloc(sizeof(struct condition));
	if (out == NULL)
	  {
	    fprintf(stderr, "%s: malloc() failed: %s",
		    myname, strerror(errno));
	    exit(1);
	  }
	if (type[0] != 'p')
	    restr_init((void *) &(out->val.re), compile_re,
		       &(out->ok), outdef);
	else
#if defined(HAVE_PCRE_H)
	    restr_init((void *) &(out->val.pcre), compile_pcre,
		       &(out->ok), outdef);
#else
	    abort();
#endif

	err = (struct condition *) malloc(sizeof(struct condition));
	if (err == NULL)
	  {
	    fprintf(stderr, "%s: malloc() failed: %s",
		    myname, strerror(errno));
	    exit(1);
	  }

	if (errdef == NULL)
	    errdef = "!."; /* Impose sane/useful default */

	if (type[0] != 'p')
	    restr_init((void *) &(err->val.re), compile_re,
		       &(err->ok), errdef);
#if defined(HAVE_PCRE_H)
	else
	    restr_init((void *) &(err->val.pcre), compile_pcre,
		       &(err->ok), errdef);
#else
	    abort();
#endif

	if (type[0] != 'p')
	    return ANALYZE_RE;
	else
	    return ANALYZE_PCRE;
      }
    else if (strcmp(type, "lnregex") == 0 || strcmp(type, "lnre") == 0
	     || strcmp(type, "lnpcre") == 0)
      {
	loadfile((type[0] != 'p') ? ANALYZE_LNRE : ANALYZE_LNPCRE,
		 outdef, &out);

	if (errdef == NULL)
	    err = NULL;
	else
	    loadfile((type[0] != 'p') ? ANALYZE_LNRE : ANALYZE_LNPCRE,
		     errdef, &err);

	if (type[0] != 'p')
	    return ANALYZE_LNRE;
	else
	    return ANALYZE_LNPCRE;
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
    void *output, *errput;
    int o, e, ok;

    assert( type == ANALYZE_RE || type == ANALYZE_PCRE );

    if (oname == NULL || ename == NULL || ofd == -1 || efd == -1)
      {
	eprint("Unable to analyze output for %s!  (Missing output)",
	       target_getname());
	return -1;
      }

    output = mapfile(2, ofd, oname, &olen);
    if (output == NULL)
	return -1;
    errput = mapfile(2, efd, ename, &elen);
    if (errput == NULL)
      {
	unmapfile(2, oname, output, olen);
	return -1;
      }
    
    ok = 0;
    if (type == ANALYZE_RE)
      {
	/* First check stdout */
	o = regexec(&(out->val.re), (char *) output, 0, NULL, 0);
	if (o != 0 && o != REG_NOMATCH)
	  {
	    /* Something bad happened */
	    char buf[1024];
	    
	    if (regerror(o, &(out->val.re), buf, 1024) != 0)
		eprint("Fatal error for %s output analysis: regexec() failed with code %s", target_getname(), buf);
	    else
		eprint("Fatal error for %s output analysis: regexec() failed with code %d", target_getname(), o);
	    ok = -1;
	  }
	else if ((o == 0 && out->ok != 0)
		 || (o == REG_NOMATCH && out->ok == 0))
	  {
	    /* Matched but shouldn't have, or the other way around */
	    ok = 1;
	    dprint("Analysis for %s: out=%d[%d] err=?[%d] ok=%d (REG_NOMATCH=%d)", target_getname(), o, out->ok, err->ok, ok, REG_NOMATCH);
	  }
	else
	  {
	    /* Stdout ok, let's check stderr */
	    e = regexec(&(err->val.re), (char *) errput, 0, NULL, 0);
	    if (e != 0 && e != REG_NOMATCH)
	      {
		/* Something bad happened */
		char buf[1024];
		
		if (regerror(e, &(err->val.re), buf, 1024) != 0)
		    eprint("Fatal error for %s output analysis: regexec() failed with code %s", target_getname(), buf);
		else
		    eprint("Fatal error for %s output analysis: regexec() failed with code %d", target_getname(), o);
		ok = -1;
	      }
	    else if ((e == 0 && err->ok != 0)
		     || (e == REG_NOMATCH && err->ok == 0))
		/* Matched but shouldn't have, or the other way around */
		ok = 1;
	    /* Summary of results */
	    dprint("Analysis for %s: out=%d[%d] err=%d[%d] ok=%d (REG_NOMATCH=%d)", target_getname(), o, out->ok, e, err->ok, ok, REG_NOMATCH);
	  }
      }
#if defined(HAVE_PCRE_H)
    else if (type == ANALYZE_PCRE)
      {
	/* First check stdout */
	o = pcre_exec(out->val.pcre, NULL, (char *) output, olen,
		      0, 0, NULL, 0);
	if (o < 0 && o != PCRE_ERROR_NOMATCH)
	  {
	    /* Something bad happened */
	    eprint("Fatal error for %s output analysis: pcre_exec() failed with code %d", target_getname(), o);
	    ok = -1;
	  }
	else if ((o >= 0 && out->ok != 0)
		 || (o == PCRE_ERROR_NOMATCH && out->ok == 0))
	  {
	    /* Matched but shouldn't have, or the other way around */
	    ok = 1;
	    dprint("Analysis for %s: out=%d[%d] err=?[%d] ok=%d (PCRE_ERROR_NOMATCH=%d)", target_getname(), o, out->ok, err->ok, ok, PCRE_ERROR_NOMATCH);
	  }
	else
	  {
	    /* Stdout ok, let's check stderr */
	    e = pcre_exec(err->val.pcre, NULL, (char *) errput, elen,
			  0, 0, NULL, 0);
	    if (e < 0 && e != PCRE_ERROR_NOMATCH)
	      {
		/* Something bad happened */
		eprint("Fatal error for %s output analysis: pcre_exec() failed with code %d", target_getname(), e);
		ok = -1;
	      }
	    else if ((e >= 0 && err->ok != 0)
		     || (e == PCRE_ERROR_NOMATCH && err->ok == 0))
		/* Matched but shouldn't have, or the other way around */
		ok = 1;
	    /* Summary of results */
	    dprint("Analysis for %s: out=%d[%d] err=%d[%d] ok=%d (PCRE_ERROR_NOMATCH=%d)", target_getname(), o, out->ok, e, err->ok, ok, PCRE_ERROR_NOMATCH);
	  }
      }
    else
	abort();
#endif

    unmapfile(2, oname, output, olen);
    unmapfile(2, ename, errput, elen);
    
    return ok;
}

/*
** analyzer_lnrun
**	Analyze a single line of output from a target according to user
**	specified regular expressions.
*/
int
analyzer_lnrun(type, what, str)
u_int type, what;
char *str;
{
    struct condition *list;
    int r;

    assert( type == ANALYZE_LNRE || type == ANALYZE_LNPCRE );
    assert( what == ANALYZE_STDOUT || what == ANALYZE_STDERR );
    assert( str != NULL );

    if (what == ANALYZE_STDOUT)
	list = out;
    else
	list = err;

    /* Special case: no condition defined == there can't be any output */
    if (list == NULL && str[0] != '\0')
	return 1;

    while (list->ok != -1)
      {
	if (type == ANALYZE_LNRE)
	  {
	    r = regexec(&(list->val.re), str, 0, NULL, 0);
	    if (r != 0 && r != REG_NOMATCH)
	      {
		/* Something bad happened */
		char buf[1024];
		
		if (regerror(r, &(list->val.re), buf, 1024) != 0)
		    eprint("Fatal error for %s output analysis: regexec() failed with code %s", target_getname(), buf);
		else
		    eprint("Fatal error for %s output analysis: regexec() failed with code %d", target_getname(), r);
		return -1;
	      }
	    else
	      {
		dprint("Analysis for %s: %d[%d] (REG_NOMATCH=%d)", target_getname(), r, list->ok, REG_NOMATCH);
		if (r == 0 && list->ok != 0)
		    /* Matched but was not supposed to! */
		    return 1;
	      }
	  }
#if defined(HAVE_PCRE_H)
	else if (type == ANALYZE_LNPCRE)
	  {
	    r = pcre_exec(list->val.pcre, NULL, str, strlen(str),
			  0, 0, NULL, 0);
	    if (r < 0 && r != PCRE_ERROR_NOMATCH)
	      {
		/* Something bad happened */
		eprint("Fatal error for %s output analysis: pcre_exec() failed with code %d", target_getname(), r);
		return -1;
	      }
	    else
	      {
		dprint("Analysis for %s: %d[%d] (PCRE_ERROR_NOMATCH=%d)", target_getname(), r, list->ok, PCRE_ERROR_NOMATCH);
		if (r >= 0 && list->ok != 0)
		    /* Matched but was not supposed to! */
		    return 1;
	      }
	  }
#endif
	else
	    abort();

	list += 1;
      }

    dprint("Analysis for %s: OK!", target_getname());

    return 0;
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
