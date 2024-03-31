#ifndef __PS_LOADSAVE_TABLES_H__
#define __PS_LOADSAVE_TABLES_H__

extern struct ps_block _ps_block_table[];
#define _ps_block_table_size (sizeof(_ps_block_table) / sizeof(struct ps_block))

extern reg_t _ps_list_table[];
#define _ps_list_table_size (sizeof(_ps_list_table) / sizeof(reg_t))

#endif /* __PS_LOADSAVE_TABLES_H__ */

