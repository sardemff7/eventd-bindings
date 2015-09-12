if ENABLE_PYTHON
eventdplugins_LTLIBRARIES += python.la
endif

python_la_SOURCES = \
	python/src/python.c \
	$(null)

python_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(PYTHON_CFLAGS) \
	$(EVENTD_CFLAGS) \
	$(null)

python_la_LIBADD = \
	libhelpers.la \
	$(PYTHON_LIBS) \
	$(EVENTD_LIBS) \
	$(null)
