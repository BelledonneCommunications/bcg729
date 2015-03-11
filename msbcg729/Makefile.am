
pluginsdir = $(libdir)/mediastreamer/plugins

plugins_LTLIBRARIES=libmsbcg729.la

libmsbcg729_la_SOURCES=bcg729_dec.c bcg729_enc.c

libmsbcg729_la_LIBADD=\
	$(ORTP_LIBS) \
	$(MEDIASTREAMER_LIBS) \
	$(top_builddir)/src/libbcg729.la

libmsbcg729_la_LDFLAGS=-module -no-undefined
AM_CPPFLAGS = -I$(top_srcdir)/include
libmsbcg729_la_CFLAGS=  $(ORTP_CFLAGS) $(MEDIASTREAMER_CFLAGS)
