#
# NAME:
# NA
#
# PURPOSE:
# NA
#
# REQUIREMENTS:
# NA
#
# INPUT:
# NA
#
# OUTPUT:
# NA
#
# NOTES:
# NA
#
# BUGS:
# NA
#
# AUTHOR:
# Øystein Godøy, DNMI/FOU, 22/06/1999
#
# MODIFIED:
# Øystein Godøy, DNMI/FOU, 10/10/2001
# Adapted for use on LINUX.
# Øystein Godøy, METNO/FOU, 07.07.2011: Adapted for new structure and new
# application name fluxval.
#
# CVS_ID:
# $Id$
#

BASEDIR = /disk1/software

FMUTILLIB = $(BASEDIR)/fmlibs/lib 
FMUTILINC = $(BASEDIR)/fmlibs/include 
FMIOLIB = $(BASEDIR)/fmlibs/lib 
FMIOINC = $(BASEDIR)/fmlibs/include 
OSIHDF5LIB = $(BASEDIR)/fmlibs/lib
OSIHDF5INC = $(BASEDIR)/fmlibs/include 
HDF5LIB =  $(BASEDIR)/osisaf_fluxdev/OSI_HL_AUX/libs/libhdf5/lib
HDF5INC =  $(BASEDIR)/osisaf_fluxdev/OSI_HL_AUX/libs/libhdf5/include
PROJLIB =  $(BASEDIR)/osisaf_fluxdev/OSI_HL_AUX/libs/PROJ.4/lib
PROJINC =  $(BASEDIR)/osisaf_fluxdev/OSI_HL_AUX/libs/PROJ.4/include

MODROOT = ./

# It is nice to include yourself.

SELFINC = ./

# Loader FLAGS required, specify as needed
LDFLAGS = \
  $(GLOB_FLG_LD) \
  -L$(FMUTILLIB) -lfmutil \
  -L$(PROJLIB) -lproj \
  -L$(OSIHDF5LIB) -losihdf5 \
  $(HDF5LIB)/libhdf5.a \
  -lz -lm 


# Turn on optimization or debugging if required.
OPT = -Wall -g

# Set CFLAGS as needed

CFLAGS = \
  -I$(SELFINC) \
  -I$(FMUTILINC) \
  -I$(OSIHDF5INC) \
  -I$(HDF5INC) \
  -I$(PROJINC) \
  $(OPT)

RUNFILE1 = \
  fluxval

RUNFILE2 = \

OBJS1 = \
  fluxval.o \
  fluxval_readobs.o \
  fluxval_stlist.o \
  return_product_area.o \
  timecnv.o 

OBJS2 = 

# Specify name of dependency files (e.g. header files)

DEPS = \
  fluxval.h
  
# Specify parameterfiles required.
# These will be installed properly if make install is executed.

PARFILES = \
  stlist_ns.txt \
  stlist_nr.txt \
  stlist_daily.txt

# Specify jobfiles as required
# These will be installed properly if make install is executed.

JOBFILES = run_job routineval

# Do not know whether this works on IRIX yet... Do not use until known...
#SRCS := $(patsubst %.o,%.c,$(OBJS))

all:
	$(MAKE) $(RUNFILE1)
#	$(MAKE) $(RUNFILE2)

$(RUNFILE1): $(OBJS1)
	$(CC) $(OBJS1) $(CFLAGS) -o $(RUNFILE1) $(LDFLAGS)

$(RUNFILE2): $(OBJS2)
	$(CC) $(OBJS2) $(CFLAGS) -o $(RUNFILE2) $(LDFLAGS)

# Specify requirements for the object generation.

$(OBJS1): $(DEPS)

$(OBJS2): $(DEPS)

clean:
	-rm -f $(OBJS1) $(OBJS2)

distclean:
	$(MAKE) rambo
	if [ -d $(MODROOT)/bin ]; then rm -rf $(MODROOT)/bin; fi
	if [ -d $(MODROOT)/job ]; then rm -rf $(MODROOT)/job; fi
	if [ -d $(MODROOT)/par ]; then rm -rf $(MODROOT)/par; fi

rambo:
	-rm -f $(OBJS1) $(OBJS2)
	-rm -f $(RUNFILE1) $(RUNFILE2)

install:
	install -d $(MODROOT)/../bin
ifdef RUNFILE1
	install $(RUNFILE1) $(MODROOT)/../bin
endif
ifdef RUNFILE2
	install $(RUNFILE2) $(MODROOT)/../bin
endif
	install -d $(MODROOT)/../job
ifdef JOBFILES
	install $(JOBFILES) $(MODROOT)/../job
endif

