## Process this file with automake to produce Makefile.in

EXTRA_DIST = gdmap.desktop.in gdmap.1

desktopdir = $(datadir)/applications
desktop_in_files=gdmap.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

#icon_DATA = gdmap_icon.png
#icondir = $(datadir)/pixmaps

man1_MANS = gdmap.1

