### configure.ac  -*- Autoconf -*-

AC_PREREQ(2.59)
AC_INIT([rawtools], [0.1], [ivan@theory.asu.ru])
AC_CONFIG_SRCDIR([lib/xform.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

## Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

## Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([locale.h stdint.h stdlib.h string.h])

## Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

## Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([setlocale strtol])
AC_OUTPUT

## I18n
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.14.2])

## Output
AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 m4/Makefile
                 po/Makefile.in
                 src/Makefile])
AC_OUTPUT

### configure.ac ends here
