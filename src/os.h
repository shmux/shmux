/*
** Copyright (C) 2002 Christophe Kalt
**
** This file is part of shmux,
** see the LICENSE file for details on your rights.
**
** $Id: os.h,v 1.3 2002-10-13 21:06:33 kalt Exp $
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

#endif
