# This is Toycluster, a code that generates an artificial cluster merger.
# Clusters consist of a DM halo defined by its density profile and a gaseous
# ICM defined by a beta model. Under the assumption of hydrostatic equillibrium
# all other quantities follow. (Donnert 2014, Donnert et al in prep.)

SHELL = /bin/bash

#OPT += -DPARABOLA       # merge in a parabola
OPT	+= -DCOMET			 # merge like a comet, ball+tail (recommended)
						 # if nothing is selected, merge as ball with R_Sample

OPT 	+= -DGIVEPARAMS		 # set beta models in parameter file

#OPT += -DSUBSTRUCTURE		 # add a population of galaxy-like subhalos
#OPT += -DSUBHOST=0			 # host subhalos in this cluster
#OPT += -DSLOW_SUBSTRUCTURE	 # put subhalos on Hernquist orbits
#OPT += -DREPORTSUBHALOS	 # print info about all subhaloes

#OPT += -DADD_THIRD_SUBHALO  # manually set the first subhalo mass, pos, vel
#OPT  += -DTHIRD_HALO_ONLY

#OPT += -DSPH_CUBIC_SPLINE 	 # for use with Gadget2

#OPT	+= -DDOUBLE_BETA_COOL_CORES # cool cores as double beta model

OPT 	+= -DNFWC_DUFFY08	 # alternate fit to concentr. param

#OPT     += -DTURB_B_FIELD    # set up a turbulent Bfield instead of a vector potential

## Target Computer ##
ifndef SYSTYPE
SYSTYPE := $(shell hostname)
endif

# standard systypes
CC       = gcc
OPTIMIZE = -Wall -g -O2
GSL_INCL = $(CPPFLAGS)
GSL_LIBS = $(LDFLAGS)
FFTW_LIBS 	=
FFTW_INCL 	=

ifeq ($(SYSTYPE),SuperMUC-NG)
CC      	=  icc
OPTIMIZE	= -O2 -g -fopenmp
GSL_INCL = $(GSL_INC)
GSL_LIBS = $(GSL_SHLIB)
FFTW_INCL= $(FFTW_INC)
FFTW_LIBS= $(FFTW_SHLIB) $(FFTW_MPI_LIB) $(FFTW_OPENMP_SHLIB)
endif

ifeq ($(SYSTYPE),DARWIN)
CC      	=  icc
OPTIMIZE	= -fast -g
GSL_INCL 	= $(CPPFLAGS)
GSL_LIBS	= -L/Users/jdonnert/Dev/lib
endif

ifeq ($(SYSTYPE),MSI)
CC      	= icc
OPTIMIZE	= -Wall -g  -O3 -xhost
GSL_INCL 	= -I/home/jonestw/donne219/Libs/$(shell hostname)/include
GSL_LIBS	= -L/home/jonestw/donne219/Libs/$(shell hostname)/lib
endif

ifeq ($(SYSTYPE),mach64.ira.inaf.it)
CC      	= gcc
OPTIMIZE	= -O2 -Wall -g  -m64 -march=native -mtune=native -mprefer-avx128 -fopenmp  -minline-all-stringops -fprefetch-loop-arrays --param prefetch-latency=300 -funroll-all-loops
GSL_INCL 	= -I/homes/donnert/Libs/include
GSL_LIBS	= -L/homes/donnert/Libs/lib
FFTW_LIBS 	= 
FFTW_INCL 	=
endif

## TARGET ##

EXEC = Toycluster

## FILES ##

SRCDIR	= src/
 
SRCFILES := ${shell find $(SRCDIR) -name \*.c -print} # all .c files in SRCDIR
OBJFILES = $(SRCFILES:.c=.o)

INCLFILES := ${shell find src -name \*.h -print} # all .h files in SRCDIR
INCLFILES += Makefile

CFLAGS 	= -std=c99 -fopenmp $(OPTIMIZE) $(OPT) $(GSL_INCL) $(FFTW_INCL)

LINK	= $(GSL_LIBS) $(FFTW_LIBS) -lm -lgsl -lgslcblas -lfftw3 

## RULES ## 

%.o : %.c
	@echo [CC] $@
	@$(CC) $(CFLAGS)  -o $@ -c $<

$(EXEC)	: $(OBJFILES)
	@echo SYSTYPE=$(SYSTYPE)
	$(CC) $(CFLAGS) $(OBJFILES) $(LINK) -o $(EXEC)
	@ctags -w $(SRCFILES) $(INCLFILES)

$(OBJFILES)	: $(INCLFILES) $(SRCFILES)

clean	: 
	rm -f  $(OBJFILES) $(EXEC)
