#
# $Id: configure.ac,v 1.5 2002-10-13 21:14:11 kalt Exp $
#
AC_PREREQ(2.53)
AC_INIT([Shell Multiplexor], [], [shmux-bugs@taranis.org], [shmux])
AC_REVISION([$Id: configure.ac,v 1.5 2002-10-13 21:14:11 kalt Exp $])
AC_CONFIG_SRCDIR([src/os.h])
AC_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_SEARCH_LIBS([tgetent], [termcap curses])
AC_SEARCH_LIBS([basename], [gen])

# Checks for header files.
AC_CHECK_HEADERS([paths.h termcap.h curses.h term.h sys/loadavg.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_C_CONST
AC_TYPE_PID_T

# Checks for library functions.
#AC_FUNC_FORK
#AC_FUNC_MALLOC
#AC_FUNC_VPRINTF
#AC_CHECK_FUNCS([alarm memset strcasecmp strchr strdup strerror])
AC_FUNC_GETLOADAVG

AC_CONFIG_FILES([src/Makefile Makefile])
AC_OUTPUT
