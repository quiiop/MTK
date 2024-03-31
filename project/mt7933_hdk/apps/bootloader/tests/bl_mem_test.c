
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include <bl_mem.h>


void usage(void)
{
    printf("Usage: <program> [ <addr> <len> <expected result> ] * N\n");
    exit(-1);
}


int main(int argc, char *argv[])
{
    bool ng = false;
    
    if (argc < 4 || (argc % 3) != 1) usage();
    argc--; argv++;

    struct mem_pair *pair;
    uint32_t        pairs;

    pair = bl_mem_get(&pairs);

    uint32_t addr;
    uint32_t len;
    int32_t  exp;
    int      ret;

    printf("RUN original\n");
    for (int i = 0; i < pairs; i++)
        printf("    %d 0x%x 0x%x\n", i, pair[i].addr, pair[i].len);

    while (argc)
    {
        addr = (uint32_t)strtoul(argv[0], NULL, 16);
        len  = (uint32_t)strtoul(argv[1], NULL, 16);
        exp  = (bool)    strtol (argv[2], NULL, 16);

        printf("RUN bl_mem_ins(0x%x, 0x%x)\n", addr, len);

        argc -= 3; argv += 3;

        ret = (int)bl_mem_ins(addr, len);
        printf("    result: %d %s %d\n", ret, ret == exp ? "=" : "!=", exp);
                
        for (int i = 0; i < pairs; i++)
            printf("    %d 0x%x 0x%x\n", i, pair[i].addr, pair[i].len);

        ng = exp == ret ? ng : true;
   }

    return ng ? -1 : 0;
}


