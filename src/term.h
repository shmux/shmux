/*
** Copyright (C) 2002-2006 Christophe Kalt
**
** This file is part of shmux
** see the LICENSE file for details on your rights.
**
** $Id: term.h,v 1.8 2006-08-10 23:49:05 kalt Exp $
*/

#if !defined(_TERM_H_)
# define _TERM_H_

void term_init(int, int, int, int, int, int);
void term_size(void);
int  tty_fd(void);
void tty_restore(void);
int  term_togglemsg(void);
int  term_toggledbg(void);
void sprint(char *, ...)
# if ( __GNUC__ == 2 && __GNUC_MINOR__ >= 5 ) || __GNUC__ >= 3
        __attribute__((__format__(__printf__, 1, 2)))
# endif
	;
void uprint(char *, ...)
# if ( __GNUC__ == 2 && __GNUC_MINOR__ >= 5 ) || __GNUC__ >= 3
        __attribute__((__format__(__printf__, 1, 2)))
# endif
	;
char *uprompt(char *, ...)
# if ( __GNUC__ == 2 && __GNUC_MINOR__ >= 5 ) || __GNUC__ >= 3
        __attribute__((__format__(__printf__, 1, 2)))
# endif
	;
void iprint(char *, ...)
# if ( __GNUC__ == 2 && __GNUC_MINOR__ >= 5 ) || __GNUC__ >= 3
        __attribute__((__format__(__printf__, 1, 2)))
# endif
	;
void dprint(char *, ...)
# if ( __GNUC__ == 2 && __GNUC_MINOR__ >= 5 ) || __GNUC__ >= 3
        __attribute__((__format__(__printf__, 1, 2)))
# endif
	;
void eprint(char *, ...)
# if ( __GNUC__ == 2 && __GNUC_MINOR__ >= 5 ) || __GNUC__ >= 3
        __attribute__((__format__(__printf__, 1, 2)))
# endif
	;

# define MSG_DEBUG       '$'
# define MSG_STDOUT      ':'
# define MSG_STDOUTTRUNC '+'
# define MSG_STDERR      '!'
# define MSG_STDERRTRUNC '*'

void tprint(char *, char, char *, ...)
# if ( __GNUC__ == 2 && __GNUC_MINOR__ >= 5 ) || __GNUC__ >= 3
        __attribute__((__format__(__printf__, 3, 4)))
# endif
	;

void nprint(char *, ...)
# if ( __GNUC__ == 2 && __GNUC_MINOR__ >= 5 ) || __GNUC__ >= 3
        __attribute__((__format__(__printf__, 1, 2)))
# endif
	;

#endif
