#
# $Id: configure.ac,v 1.2 2002-07-06 18:07:56 kalt Exp $
#
AC_PREREQ(2.53)
AC_INIT([Shell Multiplexor], [], [shmux-bugs@taranis.org], [shmux])
AC_REVISION([$Id: configure.ac,v 1.2 2002-07-06 18:07:56 kalt Exp $])
AC_CONFIG_SRCDIR([src/os.h])
AC_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_SEARCH_LIBS([tgetent], [termcap curses ncurses])

# Checks for header files.
AC_CHECK_HEADERS([paths.h termcap.h sys/loadavg.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_C_CONST
AC_TYPE_PID_T
AC_DECL_SYS_SIGLIST

# Checks for library functions.
#AC_FUNC_FORK
#AC_FUNC_MALLOC
#AC_FUNC_VPRINTF
#AC_CHECK_FUNCS([alarm memset strcasecmp strchr strdup strerror])
AC_FUNC_GETLOADAVG

# Check whether this is Solaris, term.h is a mess there..
AC_MSG_CHECKING([whether this system is running Solaris])
if test "x`(uname) 2>/dev/null`" = "xSunOS" &&
  uname -r 2>/dev/null | grep "^5" >/dev/null; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(SOLARIS, 1, [Define if the system is running Sun Solaris])
else
  AC_MSG_RESULT(no)
fi

AC_CONFIG_FILES([src/Makefile Makefile])
AC_OUTPUT
