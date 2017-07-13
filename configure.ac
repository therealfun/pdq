dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/argument.c)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LEX
AC_PROG_MAKE_SET

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_UID_T

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(gethostname mkdir mkfifo regcomp select socket strerror strstr strtol)

GTK_OBJS=xpdq
GTK_INSTALL_OBJS=xpdq_install

AM_PATH_GLIB_2_0(, , GTK_OBJS='' GTK_INSTALL_OBJS=''
	AC_MSG_WARN(Warning. Glib >= 2.0 is not installed.  
	
	*** xpdq will not be built ***
	
	) )
AM_PATH_GTK_2_0(, , GTK_OBJS='' GTK_INSTALL_OBJS=''
	AC_MSG_WARN(Warning. GTK >= 2.0 is not installed.  
	
	*** xpdq will not be built ***
	
	) )

AC_SUBST(GTK_OBJS)
AC_SUBST(GTK_INSTALL_OBJS)

dnl Checks for alternate resource locations

pdqlibdir="/etc/pdq"
AC_ARG_ENABLE(pdqlibdir, [
  --enable-pdqlibdir=DIR  place to install drivers and interfaces;
                          does not affect behavior of executables],
   if test $enableval != "no"; then
      pdqlibdir=$enableval
   fi)
AC_SUBST(pdqlibdir)

printrc_file="$pdqlibdir/printrc"
AC_ARG_ENABLE(printrc, [
  --enable-printrc=FILE   optional substitution for /etc/pdq/printrc ],
   if test $enableval != "no"; then
      printrc_file=$enableval
   fi)
AC_SUBST(printrc_file)

AC_OUTPUT(Makefile src/Makefile lpd/Makefile doc/Makefile \
	doc/pdq.1 doc/printrc.5 doc/xpdq.1 etc/Makefile etc/printrc.example)

AC_MSG_RESULT([
   Setting pdq resource directory to $pdqlibdir
   Setting global pdq configuration file to $printrc_file

   Now do 
      % make 
      # make install
      # cp $pdqlibdir/printrc.example $printrc_file

])
