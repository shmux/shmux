#
# $Id: configure.ac,v 1.10 2004-07-18 00:23:42 kalt Exp $
#
AC_PREREQ(2.53)
AC_INIT([Shell Multiplexor], [], [shmux-bugs@taranis.org], [shmux])
AC_REVISION([$Id: configure.ac,v 1.10 2004-07-18 00:23:42 kalt Exp $])
AC_CONFIG_SRCDIR([src/os.h])
AC_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# 
AC_ARG_WITH([pcre], AC_HELP_STRING([--with-pcre=DIR], [Support Perl Compatible Regular Expressions (default)]))
AC_ARG_WITH([pcre], AC_HELP_STRING([--without-pcre], [Do not support Perl Compatible Regular Expressions]))
if test "x$with_pcre" != "xno"; then
   if test "x$with_pcre" != "xyes" -a "x$with_pcre" != "x"; then
      if test ! -d "$with_pcre"; then
	 AC_MSG_ERROR([Invalid path for pcre: $with_pcre])
      fi
      CPPFLAGS="$CPPFLAGS -I$with_pcre/include"
      LDFLAGS="$LDFLAGS -L$with_pcre/lib"
   fi
fi

# Checks for libraries.
AC_SEARCH_LIBS([tgetent], [termcap curses])
AC_SEARCH_LIBS([basename], [gen])
if test "x$with_pcre" != "xno"; then
   AC_SEARCH_LIBS([pcre_compile], [pcre], ,
	AC_MSG_WARN([Perl Compatible Regular Expressions library is missing.])
	with_pcre="no")
fi

# Checks for header files.
AC_CHECK_HEADERS([libgen.h paths.h termcap.h curses.h term.h sys/loadavg.h])
if test "x$with_pcre" != "xno"; then
   AC_CHECK_HEADERS([pcre.h])
fi

# Checks for typedefs, structures, and compiler characteristics.
#AC_C_CONST
AC_TYPE_PID_T
#AH_TEMPLATE([HAVE_SYS_SIGNAME], [/* Define to 1 if you have the sys_signame[] array defined. */])
AC_CHECK_DECLS([sys_signame], , , [#include <signal.h>])

# Checks for library functions.
#AC_FUNC_FORK
#AC_FUNC_MALLOC
#AC_FUNC_VPRINTF
AC_CHECK_FUNCS([basename getloadavg])

AC_CONFIG_FILES([src/Makefile Makefile])
AC_OUTPUT
