#
# Makefile for libwifitest
#

# definitions
# ------------------------------------------------------------------------------

# wifitest tool info
PROGRAM				 = wifitest

LDFLAGS := -lutil -lm

#CFLAGS  := -DCONFIG_YOCTO_EEPROM_PATH

# compiling and linking
# ------------------------------------------------------------------------------
all: $(PROGRAM)
clean:
	-rm -f $(PROGRAM)
$(PROGRAM): main.c lib.c libwifitest.c  libtbtest.c 
	$(CC) ${CFLAGS} -Wall -o $@ $< lib.c libwifitest.c libtbtest.c  $(LDFLAGS)
