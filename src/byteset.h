/*
** Copyright (C) 2003 Christophe Kalt
**
** This file is part of shmux
** see the LICENSE file for details on your rights.
**
** $Id: byteset.h,v 1.1 2003-01-05 17:40:32 kalt Exp $
*/

#if !defined(_BYTESET_H_)
# define _BYTESET_H_

#define BSET_ERROR	0
#define BSET_SHOW	1

void byteset_init(int, char *);
int  byteset_test(int, int);

#endif
