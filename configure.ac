#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([libmaxminddb], [0.4.0], [support@maxmind.com])
AC_CONFIG_SRCDIR([include/maxminddb.h])
AC_CONFIG_HEADERS([config.h])

LT_INIT
AM_INIT_AUTOMAKE(m4_esyscmd([case `automake --version | head -n 1` in
                             *1.11*);;
                             *) echo serial-tests;;
                             esac]))
AC_PROG_LIBTOOL
# Checks for programs.
AC_PROG_CC_C99

CFLAGS=`echo ${CFLAGS} | set 's/-O2//'`
CXXFLAGS=`echo ${CXXFLAGS} | set 's/-O2//'`

AC_C_RESTRICT

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h string.h sys/time.h unistd.h stdlib.h stdint.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINTPTR_T

# Checks for library functions.

AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([memchr strtol memset gettimeofday strdup memmem])

AC_ARG_ENABLE(
        [debug],
        [  --enable-debug    Turn on debugging],
        [case "${enableval}" in
          yes) debug=true ;;
          no)  debug=false ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
        esac],[debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 bin/Makefile
                 t/Makefile])
AC_OUTPUT
