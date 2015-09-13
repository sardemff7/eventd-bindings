AC_PREREQ([2.65])
AC_INIT([eventd-bindings],[0.1],[],[])

AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_SRCDIR([ruby/src/ruby.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.aux])

AM_INIT_AUTOMAKE([1.11 foreign subdir-objects parallel-tests dist-xz no-dist-gzip tar-ustar])

AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_CXX
LT_INIT([disable-static pic-only])
PKG_PROG_PKG_CONFIG


AC_PROG_INSTALL


AC_DEFINE([EVENTD_BINDINGS_EXPORT], [__attribute__((visibility("default")))], [Export symbol])

AM_CPPFLAGS=""
AM_CFLAGS="-fvisibility=hidden -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
AM_CXXFLAGS="-fvisibility=hidden -std=gnu++11"
AM_LDFLAGS=""


if test -d .git; then
    PACKAGE_VERSION="${PACKAGE_VERSION} - `git describe --tags --always` (`git describe --tags --always --all | sed -e 's:heads/::'`)"
    AC_DEFINE_UNQUOTED([PACKAGE_VERSION], ["${PACKAGE_VERSION}"], [Full version with git information])
fi


#
# Directories
#

# package-specific dirs
pkglibdir='${libdir}/'${PACKAGE}
pkgincludedir='${includedir}/'${PACKAGE}
pkgdatadir='${datadir}/'${PACKAGE}
AC_SUBST([pkglibdir])
AC_SUBST([pkgincludedir])
AC_SUBST([pkgdatadir])

# cheat dir
bindingsdir='${pkglibdir}'
AC_SUBST([bindingsdir])

# pkg-config files
PKG_NOARCH_INSTALLDIR


PKG_CHECK_MODULES([EVENTD], [libeventd-plugin libeventd-event gobject-2.0 glib-2.0])

AC_ARG_WITH([eventdpluginsdir], AS_HELP_STRING([--with-eventdpluginsdir=DIR], [Directory for eventd plugins]), [], [with_eventdpluginsdir=yes])
case "${with_eventdpluginsdir}" in
    no|"") AC_MSG_ERROR([*** You must define eventdpluginsdir]) ;;
    yes) eventdpluginsdir="`${PKG_CONFIG} --variable=pluginsdir libeventd-plugin`" ;;
    *) eventdpluginsdir="${with_eventdpluginsdir}" ;;
esac
AC_SUBST([eventdpluginsdir])

AC_ARG_ENABLE([ruby], AS_HELP_STRING([--disable-ruby], [Disable Ruby plugin]), [], [enable_ruby=yes])
if test x${enable_ruby} = xyes; then
    PKG_CHECK_MODULES([RUBY], [ruby-2.2])
fi
AM_CONDITIONAL([ENABLE_RUBY], [test x${enable_ruby} = xyes])

AC_ARG_ENABLE([lua], AS_HELP_STRING([--disable-lua], [Disable Lua plugin]), [], [enable_lua=yes])
if test x${enable_lua} = xyes; then
    PKG_CHECK_MODULES([LUA], [lua gmodule-2.0])
    AC_DEFINE([LUA_NEEDS_GLOBAL_LOADING], [1], [Whether Lua needs loading])
fi
AM_CONDITIONAL([ENABLE_LUA], [test x${enable_lua} = xyes])

AC_ARG_ENABLE([python], AS_HELP_STRING([--disable-python], [Disable Python 3 plugin]), [], [enable_python=yes])
if test x${enable_python} = xyes; then
    PKG_CHECK_MODULES([PYTHON], [pygobject-3.0 python3 >= 3.3 gmodule-2.0])
    AC_ARG_VAR([PYTHON], [Python interpreter])
    AC_PATH_PROG([PYTHON], [python3 python])
    PYTHON_LIBNAME=`${PYTHON} -c 'import sysconfig; print(sysconfig.get_config_var("LDLIBRARY"));'`
    AC_DEFINE_UNQUOTED([PYTHON_LIBNAME], ["${PYTHON_LIBNAME}"], [Python library name to load])
    AC_DEFINE([PYTHON_NEEDS_GLOBAL_LOADING], [1], [Whether Python needs loading])
fi
AM_CONDITIONAL([ENABLE_PYTHON], [test x${enable_python} = xyes])


AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

AM_SUBST_NOTMAKE([LIBS])
AC_SUBST([shrext_cmds])

AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT

AC_MSG_RESULT([
    ${PACKAGE_NAME} ${PACKAGE_VERSION}

    Languages tests:
        Ruby:   ${enable_ruby}
        Lua:    ${enable_lua}
        Python: ${enable_python}

dnl     Tests: ${enable_tests}
])
