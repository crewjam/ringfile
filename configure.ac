#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libringfile], [0.9])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([include/ringfile.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile include/Makefile])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([ftruncate])

AC_OUTPUT
