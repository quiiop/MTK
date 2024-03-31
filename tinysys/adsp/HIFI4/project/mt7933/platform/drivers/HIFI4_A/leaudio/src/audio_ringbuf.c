#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "audio_ringbuf.h"
#include "leaudio_debug.h"
#include "le_main.h"
#include "mtk_heap.h"
#include "audio_leaudio_type.h"
/*
 * function for get how many data is available
 * @return how many data exist
 */
LE_SECTION_FUNC unsigned int RingBuf_getDataCount(const struct RingBuf *RingBuf1)
{
    return RingBuf1->datacount;
}

/*
 *
 * function for get how free space available
 * @return how free sapce
 */

LE_SECTION_FUNC unsigned int RingBuf_getFreeSpace(const struct RingBuf *RingBuf1)
{
    return RingBuf1->bufLen - RingBuf1->datacount;
}

/**
 * copy count number bytes from ring buffer to buf
 * @param buf buffer copy from
 * @param RingBuf1 buffer copy to
 * @param count number of bytes need to copy
 */
LE_SECTION_FUNC void RingBuf_copyToLinear(char *buf, struct RingBuf *RingBuf1, unsigned int count)
{
    if (count == 0)
        return;

    if (RingBuf_getDataCount(RingBuf1) < count) {
        LE_LOG_W("RingBuf_getDataCount(RingBuf1) %d < count %d\n",
                 RingBuf_getDataCount(RingBuf1), count);
        return;
    }

    if (RingBuf1->pRead < RingBuf1->pWrite) {
        memcpy(buf, RingBuf1->pRead, count);
        RingBuf1->pRead += count;
        if (RingBuf1->pRead >= RingBuf1->pBufEnd)
            RingBuf1->pRead -= RingBuf1->bufLen;
    } else {
        unsigned int r2e = RingBuf1->pBufEnd - RingBuf1->pRead;

        if (count <= r2e) {
            memcpy(buf, RingBuf1->pRead, count);
            RingBuf1->pRead += count;
            if (RingBuf1->pRead >= RingBuf1->pBufEnd)
                RingBuf1->pRead -= RingBuf1->bufLen;
        } else {
            memcpy(buf, RingBuf1->pRead, r2e);
            memcpy(buf + r2e, RingBuf1->pBufBase, count - r2e);
            RingBuf1->pRead = RingBuf1->pBufBase + count - r2e;
        }
    }
    RingBuf1->datacount -= count;
    Ringbuf_Check(RingBuf1);
}

/**
 * copy count number bytes from buf to RingBuf1
 * @param RingBuf1 ring buffer copy from
 * @param buf copy to
 * @param count number of bytes need to copy
 */

LE_SECTION_FUNC void RingBuf_copyFromLinear(struct RingBuf *RingBuf1, const char *buf,
        unsigned int count)
{
    int spaceIHave;
    char *end = RingBuf1->pBufBase + RingBuf1->bufLen;
    /* count buffer data I have */
    spaceIHave = RingBuf_getFreeSpace(RingBuf1);

    /* if not enough, assert */
    /* ASSERT(spaceIHave >= count); */
    if (spaceIHave < count) {
        LE_LOG_W("spaceIHave %d < count %d\n", spaceIHave, count);
        return;
    }

    if (RingBuf1->pRead <= RingBuf1->pWrite) {
        int w2e = end - RingBuf1->pWrite;

        if (count <= w2e) {
            memcpy(RingBuf1->pWrite, buf, count);
            RingBuf1->pWrite += count;
            if (RingBuf1->pWrite >= end)
                RingBuf1->pWrite -= RingBuf1->bufLen;
        } else {
            memcpy(RingBuf1->pWrite, buf, w2e);
            memcpy(RingBuf1->pBufBase, buf + w2e, count - w2e);
            RingBuf1->pWrite = RingBuf1->pBufBase + count - w2e;
        }
    } else {
        memcpy(RingBuf1->pWrite, buf, count);
        RingBuf1->pWrite += count;
        if (RingBuf1->pWrite >= RingBuf1->pBufEnd)
            RingBuf1->pWrite -= RingBuf1->bufLen;
    }
    RingBuf1->datacount += count;
    Ringbuf_Check(RingBuf1);
}

/**
 * copy ring buffer from RingBufs(source) to RingBuft(target)
 * @param RingBuft ring buffer copy to
 * @param RingBufs copy from copy from
 */
LE_SECTION_FUNC void RingBuf_copyFromRingBufAll(struct RingBuf *RingBuft,
        struct RingBuf *RingBufs)
{
    /* if not enough, assert */
    /* ASSERT(RingBuf_getFreeSpace(RingBuft) >= */
    /* RingBuf_getDataCount(RingBufs)); */
    if (RingBuf_getFreeSpace(RingBuft) < RingBuf_getDataCount(RingBufs))
        LE_LOG_W(
            "FreeSpace(RingBuft) %d < DataCount(RingBufs) %d\n",
            RingBuf_getFreeSpace(RingBuft),
            RingBuf_getDataCount(RingBufs));

    if (RingBufs->pRead <= RingBufs->pWrite) {
        RingBuf_copyFromLinear(RingBuft, RingBufs->pRead,
                               RingBufs->pWrite - RingBufs->pRead);
    } else {
        RingBuf_copyFromLinear(RingBuft, RingBufs->pRead,
                               RingBufs->pBufEnd - RingBufs->pRead);
        RingBuf_copyFromLinear(RingBuft, RingBufs->pBufBase,
                               RingBufs->pWrite - RingBufs->pBufBase);
    }
    RingBufs->pRead = RingBufs->pWrite;
    Ringbuf_Check(RingBuft);
    Ringbuf_Check(RingBufs);
}

/**
 * copy ring buffer from RingBufs(source) to RingBuft(target) with count
 * @param RingBuft ring buffer copy to
 * @param RingBufs copy from copy from
 */
LE_SECTION_FUNC int RingBuf_copyFromRingBuf(struct RingBuf *RingBuft, struct RingBuf *RingBufs,
        unsigned int count)
{
    if (count == 0)
        return 0;

    /* if not enough, assert */
    /* ASSERT(RingBuf_getDataCount(RingBufs) >= count && */
    /* RingBuf_getFreeSpace(RingBuft) >= count); */
    if ((RingBuf_getDataCount(RingBufs) < count) ||
            (RingBuf_getFreeSpace(RingBuft) < count)) {
        LE_LOG_W("Space RingBuft %d || Data RingBufs %d < count %d\n",
                 RingBuf_getFreeSpace(RingBuft),
                 RingBuf_getDataCount(RingBufs), count);
    }
    if (RingBufs->pRead < RingBufs->pWrite) {
        RingBuf_copyFromLinear(RingBuft, RingBufs->pRead, count);
        RingBufs->pRead += count;
        if (RingBufs->pRead >= RingBufs->pBufEnd)
            RingBufs->pRead -= RingBufs->bufLen;
    } else {
        unsigned int r2e = RingBufs->pBufEnd - RingBufs->pRead;

        if (r2e >= count) {
            RingBuf_copyFromLinear(RingBuft, RingBufs->pRead,
                                   count);
            RingBufs->pRead += count;
            if (RingBufs->pRead >= RingBufs->pBufEnd)
                RingBufs->pRead -= RingBufs->bufLen;
        } else {
            RingBuf_copyFromLinear(RingBuft, RingBufs->pRead, r2e);
            RingBuf_copyFromLinear(RingBuft, RingBufs->pBufBase,
                                   count - r2e);
            RingBufs->pRead = RingBufs->pBufBase + count - r2e;
        }
    }
    Ringbuf_Check(RingBuft);
    Ringbuf_Check(RingBufs);
    return count;
}

/**
 * write bytes size of count with value
 * @param RingBuf1 ring buffer copy to
 * @value value put into buffer
 * @count bytes ned to put.
 */

LE_SECTION_FUNC void RingBuf_writeDataValue(struct RingBuf *RingBuf1, const char value,
        const unsigned int count)
{
    if (count == 0)
        return;

    /* if not enough, assert */
    if (RingBuf_getFreeSpace(RingBuf1) < count) {
        LE_LOG_W("RingBuf_getFreeSpace(RingBuf1) %d < count %d\n",
                 RingBuf_getFreeSpace(RingBuf1), count);
    }
    if (RingBuf1->pRead <= RingBuf1->pWrite) {
        unsigned int w2e = RingBuf1->pBufEnd - RingBuf1->pWrite;

        if (count <= w2e) {
            memset(RingBuf1->pWrite, value, count);
            RingBuf1->pWrite += count;
            if (RingBuf1->pWrite >= RingBuf1->pBufEnd)
                RingBuf1->pWrite -= RingBuf1->bufLen;
        } else {
            memset(RingBuf1->pWrite, value, w2e);
            memset(RingBuf1->pBufBase, value, count - w2e);
            RingBuf1->pWrite = RingBuf1->pBufBase + count - w2e;
        }
    } else {
        memset(RingBuf1->pWrite, value, count);
        RingBuf1->pWrite += count;
        if (RingBuf1->pWrite >= RingBuf1->pBufEnd)
            RingBuf1->pWrite -= RingBuf1->bufLen;
    }
    RingBuf1->datacount += count;
}

LE_SECTION_FUNC void RingBuf_update_writeptr(struct RingBuf *RingBuf1, unsigned int count)
{
    if (count == 0)
        return;

    if (RingBuf1->pRead <= RingBuf1->pWrite) {
        unsigned int w2e = RingBuf1->pBufEnd - RingBuf1->pWrite;

        if (count <= w2e) {
            RingBuf1->pWrite += count;
            if (RingBuf1->pWrite >= RingBuf1->pBufEnd)
                RingBuf1->pWrite -= RingBuf1->bufLen;
        } else {
            RingBuf1->pWrite = RingBuf1->pBufBase + count - w2e;
            if (RingBuf1->pWrite > RingBuf1->pRead)
                RingBuf1->pWrite = RingBuf1->pBufBase;
        }
    } else {
        RingBuf1->pWrite += count;
        if (RingBuf1->pWrite >= RingBuf1->pBufEnd)
            RingBuf1->pWrite -= RingBuf1->bufLen;
    }
    /* handle buffer overflow*/
    if (RingBuf1->datacount + count > RingBuf1->bufLen) {
        LE_LOG_W("overflow count[%u] datacount[%d] Len[%d]\n",
                 count,
                 RingBuf1->datacount, RingBuf1->bufLen);

        if (RingBuf1->pRead >= RingBuf1->pWrite)
            RingBuf1->datacount =
                RingBuf1->pRead - RingBuf1->pWrite;
        else
            RingBuf1->datacount =
                RingBuf1->pWrite + RingBuf1->bufLen - RingBuf1->pRead;
    } else
        RingBuf1->datacount += count;

    Ringbuf_Check(RingBuf1);
}

LE_SECTION_FUNC void RingBuf_update_readptr(struct RingBuf *RingBuf1, unsigned int count)
{
    if (count == 0)
        return;

    if (RingBuf1->pRead <= RingBuf1->pWrite) {
        RingBuf1->pRead += count;
        if (RingBuf1->pRead >= RingBuf1->pBufEnd)
            RingBuf1->pRead -= RingBuf1->bufLen;
    } else {
        unsigned int r2e = RingBuf1->pBufEnd - RingBuf1->pRead;

        if (count <= r2e) {
            RingBuf1->pRead += count;
            if (RingBuf1->pRead >= RingBuf1->pBufEnd)
                RingBuf1->pRead -= RingBuf1->bufLen;
        } else {
            RingBuf1->pRead = RingBuf1->pBufBase + count - r2e;
            if (RingBuf1->pRead >= RingBuf1->pBufEnd)
                RingBuf1->pRead -= RingBuf1->bufLen;
        }
    }

    /* handle buffer underflow*/
    if (count > RingBuf1->datacount) {
        LE_LOG_W("underflow count %u datacount %d Len %d\n",
                 count,
                 RingBuf1->datacount, RingBuf1->bufLen);

        if (RingBuf1->pWrite >= RingBuf1->pRead)
            RingBuf1->datacount =
                RingBuf1->pWrite - RingBuf1->pRead;
        else
            RingBuf1->datacount =
                RingBuf1->pRead + RingBuf1->bufLen - RingBuf1->pWrite;
    } else
        RingBuf1->datacount -= count;
    Ringbuf_Check(RingBuf1);
}

LE_SECTION_FUNC int init_ring_buf(struct RingBuf *buf, char *vaaddr, int size)
{
    if (buf == NULL) {
        LE_LOG_E("buf == NULL\n");
        return -1;
    } else if (vaaddr == NULL) {
        LE_LOG_E("vaaddr == NULL\n");
        return -1;
    } else if (size == 0) {
        LE_LOG_E("size == 0\n");
        return -1;
    }

    buf->pBufBase = vaaddr;
    buf->pBufEnd = buf->pBufBase + size;
    buf->pRead = vaaddr;
    buf->pWrite = vaaddr;
    buf->bufLen = size;
    buf->datacount = 0;
    return 0;
}


/* check if ringbur read write pointer */
LE_SECTION_FUNC void Ringbuf_Check(struct RingBuf *RingBuf1)
{
    if (RingBuf1->pRead  ==  RingBuf1->pWrite) {
#ifdef RINGBUF_COUNT_CHECK
        if (RingBuf1->datacount != 0 && RingBuf1->datacount
                != RingBuf1->bufLen) {
            dump_ring_bufinfo(RingBuf1);
            configASSERT(0);
        }
#endif
    } else if (RingBuf1->pWrite > RingBuf1->pRead) {
#ifdef RINGBUF_COUNT_CHECK
        if ((RingBuf1->pWrite - RingBuf1->pRead)
                != RingBuf1->datacount) {
            dump_ring_bufinfo(RingBuf1);
            configASSERT(0);
        }
#endif
    } else if (RingBuf1->pRead > RingBuf1->pWrite) {
#ifdef RINGBUF_COUNT_CHECK
        if ((RingBuf1->bufLen - (RingBuf1->pRead - RingBuf1->pWrite))
                != RingBuf1->datacount) {
            dump_ring_bufinfo(RingBuf1);
            configASSERT(0);
        }
#endif
    }
    if (RingBuf1->pWrite < RingBuf1->pBufBase ||
            RingBuf1->pWrite > RingBuf1->pBufEnd) {
        dump_rbuf(RingBuf1);
        configASSERT(0);
    }
    if (RingBuf1->pRead < RingBuf1->pBufBase ||
            RingBuf1->pRead > RingBuf1->pBufEnd) {
        dump_rbuf(RingBuf1);
        configASSERT(0);
    }
    if (RingBuf1->datacount < 0) {
        dump_ring_bufinfo(RingBuf1);
        configASSERT(0);
    }
}

LE_SECTION_FUNC void RingBuf_Reset(struct RingBuf *RingBuf1)
{
    if (RingBuf1 == NULL)
        return;

    /* clear ringbuufer data*/
    memset(RingBuf1->pBufBase, 0, RingBuf1->bufLen);
    RingBuf1->pRead = RingBuf1->pBufBase;
    RingBuf1->pWrite = RingBuf1->pBufBase;
    RingBuf1->datacount = 0;
}

LE_SECTION_FUNC int RingBuf_Clear(struct RingBuf *RingBuf1)
{
    if (RingBuf1 == NULL)
        return -1;


    if (RingBuf1->pBufBase != NULL && RingBuf1->pBufEnd != NULL) {
        /* clear ringbuufer data*/
        memset(RingBuf1->pBufBase, 0, RingBuf1->bufLen);

        RingBuf1->pBufBase = NULL;
        RingBuf1->pBufEnd = NULL;
        RingBuf1->pRead = NULL;
        RingBuf1->pWrite = NULL;
        RingBuf1->bufLen = 0;
    }
    return 0;
}

LE_SECTION_FUNC void dump_rbuf(struct RingBuf *ring_buffer)
{
    if (ring_buffer == NULL)
        return;

    LE_LOG_D("Base[%p] End[%p] R[%p] w[%p] Len[%d] count[%d]\n",
             ring_buffer->pBufBase,
             ring_buffer->pBufEnd,
             ring_buffer->pRead,
             ring_buffer->pWrite,
             ring_buffer->bufLen,
             ring_buffer->datacount);

    return;
}

LE_SECTION_FUNC void dump_ring_bufinfo(struct RingBuf *buf)
{
    LE_LOG_D(
        "pBufBase = %p pBufEnd = %p  pread = %p p write = %p DataCount = %u freespace = %u\n",
        buf->pBufBase, buf->pBufEnd, buf->pRead, buf->pWrite,
        RingBuf_getDataCount(buf), RingBuf_getFreeSpace(buf));
}

