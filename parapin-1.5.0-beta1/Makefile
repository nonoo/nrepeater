#    This file is part of the package "parapin".
#
#    The parapin package is free software; you can redistribute it
#    and/or modify it under the terms of the GNU Library General Public
#    License (LGPL) as published by the Free Software Foundation.
#
#    The parapin package is distributed in the hope that it will be
#    useful, but WITHOUT ANY WARRANTY; without even the implied
#    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#    See the GNU Library General Public License for more details.
#
#    You should have received a copy of the GNU Library General Public
#    License along with parapin; if not, write to the Free
#    Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#    02111-1307 USA
#
#
#    For futher information on the parapin package, please refer to the
#    project information hosted on Sourceforge --
#
#    http://sourceforge.net/projects/parapin/
#
#
#
# Parapin Makefile-2.6 -- build parapin for 2.6 kernels
#
# original author Jeremy Elson, First released 30 March 2000
#
# Currently maintained by Al Hooton (al@hootons.org)
#
# $Id: Makefile-2.6,v 1.8 2007/01/14 21:09:24 ahooton Exp $
#

# Set this to be the location of your Linux kernel header files (and
# modules, if you're using the kernel modules).  If you are building
# for the kernel you are running, the lines below with "uname" in the
# paths will usually work.  If you're installing the libs or kernel modules,
# you will have to run make as root.  If you are building for a
# different kernel than you are running, you will need to edit these
# paths to explicitly point to the target kernel source and lib trees.
# You will also need to edit these manually if you're running an SMP
# kernel.
LINUX_SRC = /usr/src/linux-`uname -r`/
LINUX_MODULES := /lib/modules/`uname -r`/kernel/

CFLAGS += -O2 -g -Wall

LIBPARAPIN_OBJS := parapin.o

EXAMPLE_DIR := examples
EXAMPLE_PROGRAMS := \
	$(EXAMPLE_DIR)/inputtest\
	$(EXAMPLE_DIR)/outputtest\
	$(EXAMPLE_DIR)/adc0831\
	$(EXAMPLE_DIR)/ppdrv-test

BINDINGS := parapin_py

###########################################################################

default: libparapin.a $(EXAMPLE_PROGRAMS)

install: libparapin.a
	/usr/bin/install -o bin -g bin -m 644 libparapin.a /usr/local/lib/
	/usr/bin/install -o bin -g bin -m 444 parapin.h /usr/local/include/


# Language bindings install
.PHONY: $(BINDINGS)-install
$(BINDINGS)-install: install
	make -C $(subst -install,,$@) install


modules: kparapin.ko parapindriver.ko

modulesinstall: modules
	/usr/bin/install -o root -g root -m 644 kparapin/kparapin.ko $(LINUX_MODULES)
	/usr/bin/install -o root -g root -m 644 parapindriver/parapindriver.ko $(LINUX_MODULES)
	/usr/bin/install -o root -g root -m 644 parapindriver.h /usr/local/include/
	/usr/bin/install -o root -g root -m 744 ppdrv_load.sh /usr/local/bin/
	depmod -q

all: libparapin.a modules $(BINDINGS) $(EXAMPLE_PROGRAMS)

allinstall: all install modulesinstall $(BINDINGS)-install

##### libraries

libparapin.a: $(LIBPARAPIN_OBJS)
	ar -cr libparapin.a $(LIBPARAPIN_OBJS)

parapin.o: parapin.c parapin.h parapin-linux.h
	$(CC) $(CFLAGS) -c parapin.c

kparapin.ko:
	cd kparapin
	make -C $(LINUX_SRC) M=`pwd`/kparapin modules
	cd ..

parapindriver.ko:
	cd parapindriver
	make -C $(LINUX_SRC) M=`pwd`/parapindriver modules
	cd ..


# Language bindings build
.PHONY: $(BINDINGS)
$(BINDINGS): libparapin.a
	make -C $@ build


# Example programs and the test suite
$(EXAMPLE_DIR)/%: $(EXAMPLE_DIR)/%.c libparapin.a parapin.h parapin-linux.h
	$(CC) $(CFLAGS) -I. -L. $< -o $@ -lparapin


clean: $(BINDINGS)-clean
	rm -f *.[oa] *.ko $(EXAMPLE_PROGRAMS)
	make -C $(LINUX_SRC) M=`pwd`/kparapin clean
	make -C $(LINUX_SRC) M=`pwd`/parapindriver clean

# Language bindings clean
.PHONY: $(BINDINGS)-clean
$(BINDINGS)-clean: 
	make -C $(subst -clean,,$@) clean

