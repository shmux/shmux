/*
** Copyright (C) 2002, 2003 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
**
** $Id$
*/

#if !defined(_OS_H_)
# define _OS_H_

/* get some help from autoconf, it's an evil world out there. */

#include "config.h"

/* include directives common to most source files */

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/param.h>
#include <sys/types.h>
#include <limits.h>

#include <string.h>

/* shmux exit codes */
#define RC_OK	 0
#define RC_QUIT	 1
#define RC_ABORT 2
#define RC_FATAL 3
#define RC_ERROR 4

#endif
