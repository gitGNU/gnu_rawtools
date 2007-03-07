
bin_PROGRAMS = rawilv rawmatrix rawxform

LDADD = $(top_builddir)/lib/librawtools.a
localedir   = $(datadir)/locale
AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/lib \
    -DLOCALEDIR=\"$(localedir)\"

rawmatrix_LDADD  = $(top_builddir)/lib/librawtools.a
## for floor () and sqrt ()
rawmatrix_LDADD += $(LIBS_LIBM)

rawilv_SOURCES = rawilv.c

rawmatrix_SOURCES = rawmatrix.c

rawxform_SOURCES = rawxform.c
