
SHELL  = /bin/bash

CFLAGS = -m32 -std=c99 -g -Iinc -I../inc

all: tests


tests: bl_mem_test


.PHONY: bl_mem_test
bl_mem_test:
	gcc $(CFLAGS)  -o $@ bl_mem_test.c ../src/bl_mem.c
	set -o pipefail && ./bl_mem_test 0x100      0x100    0 | sed 's/^/test case 01 /'
	set -o pipefail && ./bl_mem_test 0xA0000000 0x100    1 | sed 's/^/test case 02 /'
	set -o pipefail && ./bl_mem_test 0xA0000100 0x100    1 | sed 's/^/test case 03 /'
	set -o pipefail && ./bl_mem_test 0x800FFF00 0x100    1 | sed 's/^/test case 04 /'
	set -o pipefail && ./bl_mem_test 0x80000000 0x100000 1 | sed 's/^/test case 05 /'
	set -o pipefail && ./bl_mem_test 0x80000100 0x100    1 \
                                     0x80000300 0x100    1 \
                                     0x80000500 0x100    1 \
                                     0x80000700 0x100    1 \
                                     0x80000900 0x100    1 \
                                     0x80000b00 0x100    1 \
                                     0x80000d00 0x100    1 \
                                     0x80000f00 0x100    0 | sed 's/^/test case 06 /'

