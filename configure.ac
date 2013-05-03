#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([libmaxminddb], [0.2], [support@maxmind.com])
AC_CONFIG_SRCDIR([libmaxminddb/MMDB.h])
AC_CONFIG_HEADERS([config.h])

LT_INIT
AM_INIT_AUTOMAKE
# Checks for programs.
AC_PROG_CC_C99

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h string.h unistd.h stdlib.h ])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile
                 libmaxminddb/Makefile
		 apps/Makefile
		 t/Makefile])
AC_OUTPUT
