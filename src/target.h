/*
** Copyright (C) 2002 Christophe Kalt
**
** This file is part of shmux
** see the LICENSE file for details on your rights.
**
** $Id: target.h,v 1.1 2002-07-04 21:44:51 kalt Exp $
*/

#if !defined(_TARGET_H_)
# define _TARGET_H_

void target_default(char *);
int target_add(char *);
int target_getmax(void);
int target_setbyname(char *);
void target_setbynum(u_int);
char *target_getname(void);
int target_getnum(void);
void target_getcmd(char **, char*);
int target_next(int);
void target_result(int);
void target_results(void);

#endif
