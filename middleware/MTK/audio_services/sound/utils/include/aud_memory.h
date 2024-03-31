#ifndef __AUD_MEMORY_H__
#define __AUD_MEMORY_H__

void aud_memory_init(void *base_addr, unsigned int base_size);
void aud_memory_uninit(void);
void *aud_memory_malloc(unsigned int buffer_size);
void aud_memory_free(void *buffer_addr);

#endif /* #ifndef __AUD_MEMORY_H__ */
