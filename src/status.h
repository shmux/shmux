/*
** Copyright (C) 2002, 2003 Christophe Kalt
**
** This file is part of shmux
** see the LICENSE file for details on your rights.
**
** $Id: status.h,v 1.2 2003-03-19 02:15:36 kalt Exp $
*/

#if !defined(_STATUS_H_)
# define _STATUS_H_

void status_init(int, int, int);
void status_spawned(int);
void status_phase(int, int);
void status_update(void);

#endif
