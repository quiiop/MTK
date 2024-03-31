#ifndef __AUDIO_RINGBUF_H__
#define __AUDIO_RINGBUF_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * real data operation with ringbuffer
 * when moving memory.
 */
struct RingBuf {
    char *pBufBase;
    char *pBufEnd;
    char *pRead;
    char *pWrite;
    int bufLen;
    int datacount;
};

unsigned int RingBuf_getDataCount(const struct RingBuf *RingBuf1);
unsigned int RingBuf_getFreeSpace(const struct RingBuf *RingBuf1);
void RingBuf_copyToLinear(char *buf, struct RingBuf *RingBuf1, unsigned int count);
void RingBuf_copyFromLinear(struct RingBuf *RingBuf1, const char *buf, unsigned int count);
void RingBuf_copyFromRingBufAll(struct RingBuf *RingBuft, struct RingBuf *RingBufs);
int RingBuf_copyFromRingBuf(struct RingBuf *RingBuft, struct RingBuf *RingBufs, unsigned int count);
void RingBuf_writeDataValue(struct RingBuf *RingBuf1, const char value, const unsigned int count);
void RingBuf_update_writeptr(struct RingBuf *RingBuf1, unsigned int count);
void RingBuf_update_readptr(struct RingBuf *RingBuf1, unsigned int count);
int init_ring_buf(struct RingBuf *buf, char *vaaddr, int size);
void Ringbuf_Check(struct RingBuf *RingBuf1);
void RingBuf_Reset(struct RingBuf *RingBuf1);
int RingBuf_Clear(struct RingBuf *RingBuf1);
void dump_rbuf(struct RingBuf *ring_buffer);
void dump_ring_bufinfo(struct RingBuf *buf);

#endif
