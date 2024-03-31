

#include "ps_loadsave.h"


#define _reg_w(reg, val) do { *(uint32_t *)reg = val; } while (0)


#define _reg_r(reg, val)  do { val = *(uint32_t *)reg; } while (0)


/**
 * Register copy - copies from or to register.
 *
 * copy 32bits in every operation.
 */
static void _regcpy(uint32_t *dst, uint32_t *src, uint32_t len)
{
	while (len) {
		*dst = *src;
		dst++; src++; len -= 4;
	}
}


void ps_reg_block_save(const struct ps_block *table, int table_size,
                       uint32_t *buffer)
{
	while (table_size--) {
		_regcpy(buffer, (uint32_t *)table->reg, (uint32_t)table->len);
		buffer += table->len / sizeof(uint32_t);
		table++;
	}
}


void ps_reg_block_load(const struct ps_block *table, int table_size,
                         uint32_t *buffer)
{
	while (table_size--) {
		_regcpy((uint32_t *)table->reg, buffer, (uint32_t)table->len);
		buffer += table->len / sizeof(uint32_t);
		table++;
	}
}


void ps_reg_table_save(const reg_t *table, uint32_t table_size,
                       uint32_t *table_buffer)
{
	while (table_size) {
		*table_buffer = *(uint32_t *)(*table);
		table_buffer++; table++; table_size--;
	}
}


void ps_reg_table_load(const reg_t *table, int table_size,
                       const uint32_t *table_buffer)
{
	while (table_size) {
		*(uint32_t *)(*table) = *table_buffer;
		table_buffer++; table++; table_size--;
	}
}

