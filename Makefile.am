ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = bcg729.spec 
SUBDIRS =  include src

if BUILD_MSBCG729
SUBDIRS += msbcg729 
endif

if RUN_TESTS
SUBDIRS += test 
endif
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbcg729.pc

rpm:
	$(MAKE) dist
	TAR_OPTIONS=--wildcards rpmbuild -ta --clean --rmsource --rmspec $(PACKAGE)-$(VERSION).tar.gz
