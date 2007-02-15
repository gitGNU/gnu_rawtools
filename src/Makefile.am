
bin_PROGRAMS = rawilv rawxform

LDADD = $(top_builddir)/lib/librawtools.a
localedir   = $(datadir)/locale
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib \
    -DLOCALEDIR=\"$(localedir)\"

rawilv_SOURCES = rawilv.c

rawxform_SOURCES = rawxform.c
