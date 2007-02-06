
bin_PROGRAMS = rawflt2u1 rawilv

LDADD = $(top_builddir)/lib/librawtools.a
localedir   = $(datadir)/locale
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib \
    -DLOCALEDIR=\"$(localedir)\"

rawflt2u1_SOURCES = rawflt2u1.c

rawilv_SOURCES = rawilv.c
