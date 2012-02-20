ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src include
if BUILD_MSBCG729
SUBDIRS += msbcg729 
endif

if RUN_TESTS
SUBDIRS += test 
endif
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbcg729.pc



