if ENABLE_LUA
eventdplugins_LTLIBRARIES += lua.la
endif

lua_la_SOURCES = \
	lua/src/lua.c \
	$(null)

lua_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LUA_CFLAGS) \
	$(EVENTD_CFLAGS) \
	$(null)

lua_la_LIBADD = \
	libhelpers.la \
	$(LUA_LIBS) \
	$(EVENTD_LIBS) \
	$(null)
