## Process this file with automake to produce Makefile.in

INCLUDES = \
	@GTK_CFLAGS@

bin_PROGRAMS = gdmap

gdmap_SOURCES = \
	main.c \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
	global.c global..h \
	tree.c tree.h \
	colors.c colors.h

gdmap_LDADD = @GTK_LIBS@

