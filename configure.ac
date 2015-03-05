AC_PREREQ([2.69])
AC_INIT([broker], [0.1], [report@bug.org])
AC_CONFIG_SRCDIR([broker.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netdb.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([memset socket])

AC_ARG_WITH(evfibers,
  [  --with-evfibers=DIR           Alternate location for libevfibers],
  CFLAGS="${CFLAGS} -I${withval}/include"; LDFLAGS="${LDFLAGS} -L${withval}/lib")

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
