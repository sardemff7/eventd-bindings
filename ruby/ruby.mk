if ENABLE_RUBY
eventdplugins_LTLIBRARIES += ruby.la
endif

ruby_la_SOURCES = \
	ruby/src/ruby.c \
	$(null)

ruby_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(RUBY_CFLAGS) \
	$(EVENTD_CFLAGS) \
	$(null)

ruby_la_LIBADD = \
	libhelpers.la \
	$(RUBY_LIBS) \
	$(EVENTD_LIBS) \
	$(null)
