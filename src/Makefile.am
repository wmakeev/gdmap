## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@UI_CFLAGS@

bin_PROGRAMS = gdmap

gdmap_SOURCES = \
	main.c \
	gui_main.c \
	dirselect.c \
	global.c \
	colors.c \
	tree.c

gdmap_LDADD = @UI_LIBS@

