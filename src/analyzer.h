/*
** Copyright (C) 2003 Christophe Kalt
**
** This file is part of shmux
** see the LICENSE file for details on your rights.
**
** $Id: analyzer.h,v 1.1 2003-03-20 01:45:30 kalt Exp $
*/

#if !defined(_ANALYZER_H_)
# define _ANALYZER_H_

#define ANALYZE_NONE 0
#define ANALYZE_RE   1
#define ANALYZE_PCRE 2
#define ANALYZE_RUN  3

int analyzer_init(char *, char *, char *);
int analyzer_run(u_int, int, char *, int, char *);
char *analyzer_cmd(void);
u_int analyzer_timeout(void);

#endif
