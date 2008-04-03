/*
** Copyright (C) 2003 Christophe Kalt
**
** This file is part of shmux
** see the LICENSE file for details on your rights.
**
** $Id$
*/

#if !defined(_ANALYZER_H_)
# define _ANALYZER_H_

#define ANALYZE_NONE   0
#define ANALYZE_RE     1
#define ANALYZE_LNRE   2
#define ANALYZE_PCRE   3
#define ANALYZE_LNPCRE 4
#define ANALYZE_RUN    5

#define ANALYZE_STDOUT 1
#define ANALYZE_STDERR 2

int analyzer_init(char *, char *, char *);
int analyzer_run(u_int, int, char *, int, char *);
int analyzer_lnrun(u_int, u_int, char *);
char *analyzer_cmd(void);
u_int analyzer_timeout(void);

#endif
