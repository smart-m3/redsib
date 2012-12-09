#############################################################################
#
#	(RED)SIB Daemon
#
#	2009 Nokia Corporation / ARCES University of Bologna
#
#############################################################################

AC_INIT()
AC_CONFIG_AUX_DIR(config)

AM_INIT_AUTOMAKE(redsibd, 0.1)
AM_CONFIG_HEADER(config.h)

#############################################################################
# Constants
#############################################################################

#############################################################################
# Localisation
#############################################################################
#GETTEXT_PACKAGE=$PACKAGE
#AC_SUBST(GETTEXT_PACKAGE)
#AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, 
#		   "$GETTEXT_PACKAGE", [Define gettext package])
#ALL_LINGUAS="en_GB"
#AM_GLIB_GNU_GETTEXT

#localedir=`$PKG_CONFIG osso-af-settings --variable=localedir`
#AC_SUBST(localedir)
#AC_DEFINE_UNQUOTED([PACKAGE_LOCALE_DIR], 
#		   ["${prefix}/${DATADIRNAME}/locale"], [Locale directory])

#############################################################################
# Checks for some common programs needed for configure/compile/install
#############################################################################

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_LIBTOOL
AC_PROG_RANLIB

#############################################################################
# Check for some header files (TODO: Add more?)
#############################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS([limits.h stdlib.h string.h unistd.h])

##############################################################################
# Check for iconv
##############################################################################
AC_CHECK_FUNCS([iconv])
AC_CHECK_HEADERS([iconv.h])
AC_CHECK_LIB([iconv],[main])

##############################################################################
# Check for pthread presence
##############################################################################
AC_CHECK_HEADERS([pthread.h],
		 [],
		 [AC_MSG_ERROR(sib-daemon needs pthread library)])
AC_CHECK_LIB([pthread],[main])



#############################################################################
# Check whether debug prints should be used
#############################################################################
AC_ARG_WITH(debug,
        AS_HELP_STRING([--with-debug],
                       [Print additional debug messages (default = no)]),
        [AC_DEFINE([WHITEBOARD_DEBUG],[1],[Print debug messages])],
        [with_debug=no]
)

AM_CONDITIONAL(DEBUG, test $with_debug = yes)

#############################################################################
# Check whether WQL should be used
#############################################################################
use_wql=no
AC_ARG_ENABLE(wql,
        AS_HELP_STRING([--enable-wql],
                       [Should Wilbur queries (and Python) be included (default = yes)]),
[
	if test $enableval = no; then
		use_wql=no
	else
		use_wql=no

	fi
	],
[
		use_wql=no
]
)

AM_CONDITIONAL(WQL, test "x$use_wql"="xno")


##############################################################################
# Check for GNOME environment
##############################################################################
PKG_CHECK_MODULES(GNOME,
[
	glib-2.0 >= 2.8.6
	dbus-1 >= 0.61
	dbus-glib-1 >= 0.61
])

AC_SUBST(GNOME_CFLAGS)
AC_SUBST(GNOME_LIBS)

PKG_CHECK_MODULES(WHITEBOARD,
[
	libm3_parse_n_gen >= 2.0-beta1
])

AC_SUBST(WHITEBOARD_CFLAGS)
AC_SUBST(WHITEBOARD_LIBS)

PKG_CHECK_MODULES(LIBSIB,
[
	libsib >= 2.0-beta1
])

AC_SUBST(LIBSIB_CFLAGS)
AC_SUBST(LIBSIB_LIBS)

#PKG_CHECK_MODULES(CONIC,
#[
#	conic >= 0.13
#])

#AC_SUBST(CONIC_FLAGS)
#AC_SUBST(CONIC_LIBS)

##############################################################################
# Checks for typedefs, structures, and compiler characteristics.
##############################################################################

AC_C_CONST
AC_C_INLINE
AC_STRUCT_TM

##############################################################################
# Locale stuff
##############################################################################

AC_DEFINE_UNQUOTED([LOCALEDIR], 
		   "${runtime_localedir}", [Runtime locale catalog files path])

##############################################################################
# Check compile target to be able to disable local player in emulator
##############################################################################

case $host in
        i386-* | i686-*)
               CFLAGS="$CFLAGS -Di686"
	       USE_i686=yes
	       USE_ARM=no
               ;;
        arm-*)
               CFLAGS="$CFLAGS -DARM"
	       USE_ARM=yes
	       USE_i686=no
               ;;
esac

AM_CONDITIONAL(ARM, test "x$USE_ARM"="xyes")
AM_CONDITIONAL(i686, test "x$USE_i686"="xyes")



##############################################################################
# Files to create
##############################################################################

AC_OUTPUT(
	Makefile \
	include/Makefile \
	src/Makefile \
	etc/Makefile 
)



echo "Happy end!..."

