/*
** Copyright (C) 2002, 2003 Christophe Kalt
**
** This file is part of shmux
** see the LICENSE file for details on your rights.
**
** $Id: loop.h,v 1.7 2003-05-03 01:13:53 kalt Exp $
*/

#if !defined(_LOOP_H_)
# define _LOOP_H_

#define OUT_NULL  0x00	/* Used internally by loop() */
#define OUT_MIXED 0x01	/* Mixed target output */
#define OUT_ATEND 0x02	/* Non-mised target output (displayed at end) */
#define OUT_IFERR 0x10	/* Output only displayed on error */
#define OUT_COPY  0x20	/* Keep copies of everything for the user */

void loop(char *, u_int, int, char *, int, char *, u_int, char *, u_int);

#endif
