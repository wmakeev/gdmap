## Process this file with automake to produce Makefile.in

bin_PROGRAMS = gdmap

gdmap_SOURCES = \
	main.c l_i18n.h \
	utils.c utils.h \
	gui_main.c \
	global.c \
	colors.c \
	tree.c

gdmap_CFLAGS = \
	-DGDMAP_LOCALE_DIR=\""$(localedir)"\" -DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" @UI_CFLAGS@

gdmap_LDADD = @UI_LIBS@

