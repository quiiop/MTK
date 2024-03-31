
#ifndef __PS_LOADSAVE_H__
#define __PS_LOADSAVE_H__


#include <stdint.h>


typedef uint32_t reg_t;


struct ps_block {
    reg_t    reg; /* address of register */
    uint16_t len; /* address length, not the number of registers */
};


void ps_reg_block_save(const struct ps_block *table, int table_size,
                       uint32_t *buffer);


void ps_reg_block_load(const struct ps_block *table, int table_size,
                       uint32_t *buffer);


void ps_reg_table_save(const reg_t *table, uint32_t table_size,
                       uint32_t *table_buffer);


void ps_reg_table_load(const reg_t *table, int table_size,
                       const uint32_t *table_buffer);


#endif /* __PS_LOADSAVE_H__ */

