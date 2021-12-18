AC_PREREQ([2.71])
AC_INIT([oas],[1.0],[https://github.com/ThomasNoll/oas/issues],[oas],[https://github.com/ThomasNoll/oas/issues
	])
AC_CONFIG_SRCDIR([src/oas.c])
AC_CONFIG_AUX_DIR([build/aux])
AC_CONFIG_MACRO_DIR([build/m4])
AM_INIT_AUTOMAKE([1.11 foreign silent-rules subdir-objects -Wall -Werror])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT([disable-static])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MKDIR_P
AC_SUBST([PREFIX])
AC_SUBST([SYSCONFDIR])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])
LT_INIT
AC_OUTPUT

