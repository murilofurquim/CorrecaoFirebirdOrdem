## Makefile.in for ICU - test/testmap
## Copyright (c) 2003, International Business Machines Corporation and
## others. All Rights Reserved.

## Source directory information
srcdir = .
top_srcdir = ../..

top_builddir = ../..

include $(top_builddir)/icudefs.mk

## Build directory information
subdir = test/testmap

## Extra files to remove for 'make clean'
CLEANFILES = *~ $(DEPS)

## Target information
TARGET = testmap$(EXEEXT)

#DEFS = -DHAVE_CONFIG_H
CPPFLAGS += -I$(top_builddir)/common -I$(top_srcdir)/common -I$(top_srcdir)/i18n 
#CFLAGS +=  -O3 $(THREADSCFLAGS)
#CXXFLAGS =  -O $(THREADSCXXFLAGS)

LIBS = $(LIBICUI18N) $(LIBICUUC) $(DEFAULT_LIBS) $(LIB_M)

# Data packaging options are set at configure time with --with-data-packaging.
CPPFLAGS += -DUDATA_DLL

#ifeq ($(PKGDATA_MODE),dll)
#CPPFLAGS += -DUDATA_MAP_DLL
#LIBICUDATA= -L$(top_srcdir)/../data -licudata

OBJECTS = testmap.o

DEPS = $(OBJECTS:.o=.d)

## List of phony targets
.PHONY : all all-local install install-local clean clean-local	\
distclean distclean-local dist dist-local check check-local

## Clear suffix list
.SUFFIXES :

## List of standard targets
Xall: all-local
install: install-local
clean: clean-local
distclean : distclean-local
dist: dist-local
check: all check-local

all-local: $(TARGET)

install-local:

dist-local:

clean-local:
	test -z "$(CLEANFILES)" || $(RMV) $(CLEANFILES)
	$(RMV) $(OBJECTS) $(TARGET)

distclean-local: clean-local
	$(RMV) Makefile

check-local: all-local
	$(INVOKE) ./$(TARGET)

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	 && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

$(TARGET) : $(OBJECTS)
	$(LINK.cc) $(OUTOPT)$@ $^ $(LIBS)

ifeq (,$(MAKECMDGOALS))
-include $(DEPS)
else
ifneq ($(patsubst %clean,,$(MAKECMDGOALS)),)
ifneq ($(patsubst %install,,$(MAKECMDGOALS)),)
-include $(DEPS)
endif
endif
endif
