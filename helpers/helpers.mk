AM_CFLAGS += \
	-I $(srcdir)/helpers/include \
	$(null)

AM_CXXFLAGS += \
	-I $(srcdir)/helpers/include \
	$(null)

noinst_LTLIBRARIES += libhelpers.la

libhelpers_la_SOURCES = \
	helpers/include/helpers.h \
	helpers/src/helpers.c \
	$(null)

libhelpers_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(EVENTD_CFLAGS) \
	$(null)

libhelpers_la_LIBADD = \
	$(EVENTD_LIBS) \
	$(null)
