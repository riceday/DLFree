#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([dlfree], [1.0], [kenny@logos.ic.i.u-tokyo.ac.jp])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([include/dlfree/dlfree.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([socket], [socket])
AC_CHECK_LIB([nsl], [gethostbyname])
AC_CHECK_LIB([expat], [XML_ParserCreate])
AC_CHECK_LIB([pthread], [pthread_create])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_HEADER_TIME
AC_TYPE_UINT64_T

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([qsort gethostbyname gethostname gettimeofday inet_ntoa memset select socket strdup strerror])

AC_CONFIG_FILES([Makefile
                 app/Makefile
                 src/Makefile
                 src/comm/Makefile
                 src/graph/Makefile
                 src/gxp/Makefile
                 src/iface/Makefile
                 src/sim/Makefile
                 src/std/Makefile
                 src/struct/Makefile
                 src/xml/Makefile])
AC_OUTPUT
