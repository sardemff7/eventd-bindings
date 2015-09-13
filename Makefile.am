comma := ,
empty :=
space := $(empty) $(empty)

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory

bindings_DATA =

eventdplugins_LTLIBRARIES =
noinst_LTLIBRARIES =

EXTRA_DIST = \
	autogen.sh \
	README.md \
	$(null)
CLEANFILES =


AM_CPPFLAGS += \
	-I $(srcdir)/src/ \
	-I $(builddir)/src/ \
	-D G_LOG_DOMAIN=\"eventd-bindings\" \
	-D EVENTD_PLUGINS_DIR=\"$(eventdpluginsdir)\" \
	$(null)

AM_LDFLAGS += \
	-avoid-version -module \
	$(null)


include helpers/helpers.mk
include ruby/ruby.mk
include lua/lua.mk
include python/python.mk

-include local-rules.mk
