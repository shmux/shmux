/*
** Copyright (C) 2002, 2003 Christophe Kalt
**
** This file is part of shmux
** see the LICENSE file for details on your rights.
**
** $Id: loop.h,v 1.9 2003-05-08 00:27:58 kalt Exp $
*/

#if !defined(_LOOP_H_)
# define _LOOP_H_

#define OUT_NULL  0x00	/* Used internally by loop() */
#define OUT_MIXED 0x01	/* Mixed target output */
#define OUT_ATEND 0x02	/* Non-mised target output (displayed at end) */
#define OUT_COPY  0x10	/* Keep copies of everything for the user */
#define OUT_IFERR 0x20	/* Output only displayed on error */
#define OUT_ERR   0x40	/* Error found in output */

void loop(char *, u_int, int, char *, int, char *, u_int, char *, int);

#endif
