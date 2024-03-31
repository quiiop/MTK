/* Copyright Statement:
 *
 * (C) 2017  Airoha Technology Corp. All rights reserved.
 *
 * This software/firmware and related documentation ("Airoha Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to Airoha Technology Corp. ("Airoha") and/or its licensors.
 * Without the prior written permission of Airoha and/or its licensors,
 * any reproduction, modification, use or disclosure of Airoha Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) Airoha Software
 * if you have agreed to and been bound by the applicable license agreement with
 * Airoha ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of Airoha Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT AIROHA SOFTWARE RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH AIROHA SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN AIROHA SOFTWARE. AIROHA SHALL ALSO NOT BE RESPONSIBLE FOR ANY AIROHA
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT AIROHA'S OPTION, TO REVISE OR REPLACE AIROHA SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
 */

#if 0
#include "task.h"
#include "semphr.h"
#include "portmacro.h"
#include "queue.h"
#endif /* #if 0 */
#include "FreeRTOS.h"
#include "semphr.h"
#include <timers.h>
#include <string.h>
//#include "syslog.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/queue.h>
#include "bt_mesh_os_layer_api.h"
#ifndef MTK_TFM_ENABLE
#include "hal_trng.h"
#else /* #ifndef MTK_TFM_ENABLE */
#include "tfm_platform_api.h"
#endif /* #ifndef MTK_TFM_ENABLE */
#include "bt_callback_manager.h"
#include "bt_type.h"
#include "nvdm.h"
#include "hal_nvic.h"

//#define TT_RM_ECC
#ifndef TT_RM_ECC
#include "uECC.h"
#endif /* #ifndef TT_RM_ECC */

#define BT_MESH_NVDM_NONBLOCK
#define BT_MESH_RECORD_DATA_CACAHE_IN_RAM

log_control_block_t log_control_block_mesh_os_layer = {
    "mesh_os_layer",
    0,
    0x64,
    (DEBUG_LOG_ON),
    (PRINT_LEVEL_ERROR),
    print_module_log,
    dump_module_buffer
};

////////////////////////////////////////////////////////////////////////////////
// Private Type Definition /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM
typedef struct {
    char *record_id;
    uint32_t id_len;
    uint8_t *data;
    uint32_t data_len;
} mesh_record_data_item_t;

typedef struct _record_node_t {
    mesh_record_data_item_t item;
    struct _record_node_t *next;
} record_node_t;

typedef struct _seq_num_record_block_t {
    uint8_t *data;
    uint32_t data_len;
} seq_num_record_block_t;
#endif /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */

typedef struct {
    LIST_HEAD(L_HEAD, list_entry) head;
    uint32_t tag;
    uint32_t count;
} MESH_OS_LAYER_DLIST;

typedef struct list_entry {
    LIST_ENTRY(list_entry) entry;
    uint32_t tag;
    void *data;
} MESH_OS_LAYER_DLIST_ENTRY;



////////////////////////////////////////////////////////////////////////////////
// Private Type Definition /////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct {
    STAILQ_HEAD(Q_HEAD, queue_entry) head;
    uint32_t tag;
    uint32_t count;
} MESH_OS_LAYER_QUEUE;


typedef struct queue_entry {
    STAILQ_ENTRY(queue_entry) entry;
    uint32_t tag;
    void *data;
} MESH_OS_LAYER_QUEUE_ENTRY;




/*******************************************
  *************memory operation***************
********************************************/

uint32_t p_test_heap = 0;
uint8_t *bt_mesh_os_layer_memory_alloc(uint16_t size)
{
    if (size == 0) {
        LOG_E(mesh_os_layer, "mesh_os_memory_alloc wrong param, size(0)\n");
        return NULL;
    }
    uint8_t *p = (uint8_t *)pvPortMalloc(size);
    if (!p) {
        LOG_E(mesh_os_layer, "mesh_os_memory_alloc fail, size(%d).\n", size);
    } else if (size >= 128) {
        LOG_W(mesh_os_layer, "mesh_os_memory_alloc success, p_addr(0x%08x), size(0x%x).\n", p, size);
    }
    p_test_heap = (uint32_t)p;
    return p;
}

void bt_mesh_os_layer_memory_free(uint8_t *p)
{
    if (p == NULL) {
        return;
    } else {
        //LOG_W(mesh_os_layer, "bt_mesh_os_layer_memory_free success, p_addr(0x%08x).\n", p);
    }
    vPortFree(p);
}

/*******************************************
  ****************List operation***************
********************************************/

////////////////////////////////////////////////////////////////////////////////
// Public Functions ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void *bt_mesh_os_layer_ds_dlist_alloc(void)
{
    MESH_OS_LAYER_DLIST *list;

    list = (MESH_OS_LAYER_DLIST *)bt_mesh_os_layer_memory_alloc(sizeof(MESH_OS_LAYER_DLIST));
    if (!list) {
        LOG_I(mesh_os_layer, "mesh_os_dlist_alloc fail.\n");
        return NULL;
    }
    list->tag = 0x19760108;
    list->count = 0;
    list->head.lh_first = NULL;

    return list;
}

void bt_mesh_os_layer_ds_dlist_free(void *list)
{
    void *dat;

    if (list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_free fail, list(NULL)\n");
        return;
    }

    while ((dat = bt_mesh_os_layer_ds_dlist_remove_head(list))) {
        bt_mesh_os_layer_ds_dlist_entry_free(dat);
    }

    bt_mesh_os_layer_memory_free(list);
}

void bt_mesh_os_layer_ds_dlist_empty(void *list)
{
    void *dat;

    if (list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_empty fail, list(NULL)\n");
        return;
    }

    while ((dat = bt_mesh_os_layer_ds_dlist_remove_head(list))) {
        bt_mesh_os_layer_ds_dlist_entry_free(dat);
    }
}

void *bt_mesh_os_layer_ds_dlist_entry_alloc(uint32_t size)
{
    MESH_OS_LAYER_DLIST_ENTRY *new_entry;

    if (size == 0) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_entry_alloc fail, size(0)\n");
        return NULL;
    }

    new_entry = (MESH_OS_LAYER_DLIST_ENTRY *) bt_mesh_os_layer_memory_alloc(sizeof(MESH_OS_LAYER_DLIST_ENTRY) + size);

    if (new_entry) {
        new_entry->tag = 0x20111118;
        return (uint8_t *)new_entry + offsetof(MESH_OS_LAYER_DLIST_ENTRY, data);
    }

    LOG_W(mesh_os_layer, "mesh_os_dlist_entry_alloc fail.\n");
    return NULL;
}

void bt_mesh_os_layer_ds_dlist_entry_free(void *dat)
{
    uint32_t *ptag;
    MESH_OS_LAYER_DLIST_ENTRY *pentry;

    if (dat == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_entry_free, dat(NULL)\n");
        return;
    }

    ptag = (uint32_t *)((uint8_t *)dat  - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, tag));
    pentry = (MESH_OS_LAYER_DLIST_ENTRY *)((uint8_t *)dat - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));

    if (*ptag == 0x20111118) { //valid entry
        bt_mesh_os_layer_memory_free((uint8_t *)pentry);
    }
}

void *bt_mesh_os_layer_ds_dlist_remove_head(void *list)
{
    void *pdat = NULL;
    MESH_OS_LAYER_DLIST_ENTRY *entry_removed;

    if (list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_head fail, list(NULL)\n");
        return NULL;
    }

    if (LIST_FIRST(&((MESH_OS_LAYER_DLIST *)list)->head)) {
        pdat = ((uint8_t *)LIST_FIRST(&((MESH_OS_LAYER_DLIST *)list)->head) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));
        entry_removed = (MESH_OS_LAYER_DLIST_ENTRY *)LIST_FIRST(&((MESH_OS_LAYER_DLIST *)list)->head);

        LIST_REMOVE(entry_removed, entry);
        ((MESH_OS_LAYER_DLIST *)list)->count--;
    }

    return pdat;
}

void *bt_mesh_os_layer_ds_dlist_remove(void *list, void *removed)
{
    uint32_t *tag_removed;
    void *pdat = removed;

    MESH_OS_LAYER_DLIST_ENTRY *entry_removed;

    if (list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_remove fail, list(NULL)\n");
        return NULL;
    }

    if (pdat == NULL) {
        pdat = bt_mesh_os_layer_ds_dlist_remove_head(list);
    } else {
        tag_removed = (uint32_t *)((uint8_t *)removed  - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, tag));

        if (*tag_removed == 0x20111118) { //valid entry
            entry_removed = (MESH_OS_LAYER_DLIST_ENTRY *)((uint8_t *)removed - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));
            LIST_REMOVE(entry_removed, entry);
            ((MESH_OS_LAYER_DLIST *)list)->count--;
        }
    }

    return pdat;
}

bool bt_mesh_os_layer_ds_dlist_delete(void *list, void *deleted)
{
    bool ret = false;

    if (list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_delete fail, list(NULL)\n");
        return ret;
    }

    if (deleted == NULL) {
        MESH_OS_LAYER_DLIST_ENTRY *head_entry = LIST_FIRST(&((MESH_OS_LAYER_DLIST *)list)->head);

        if (head_entry) {
            LIST_REMOVE(head_entry, entry);
            ((MESH_OS_LAYER_DLIST *)list)->count--;
            return true;
        }
    } else {
        ret = bt_mesh_os_layer_ds_dlist_remove(list, deleted);

        if (ret) {
            bt_mesh_os_layer_ds_dlist_entry_free(deleted);
        }
    }

    return ret;
}

void bt_mesh_os_layer_ds_dlist_insert(void *list, void *inserted)
{
    uint32_t *tag_inserted;
    MESH_OS_LAYER_DLIST_ENTRY *entry_inserted;

    if (list == NULL || inserted == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_insert fail, list = %p, inserted = %p\n", list, inserted);
        return;
    }

    tag_inserted = (uint32_t *)((uint8_t *)inserted  - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, tag));

    if (*tag_inserted == 0x20111118) { //valid entry
        entry_inserted = (MESH_OS_LAYER_DLIST_ENTRY *)((uint8_t *)inserted - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));
        LIST_INSERT_HEAD(&((MESH_OS_LAYER_DLIST *)list)->head, entry_inserted, entry);
        ((MESH_OS_LAYER_DLIST *)list)->count++;
    }
}

void bt_mesh_os_layer_ds_dlist_insert_after(void *list, void *inlist, void *inserted)
{
    uint32_t *tag_inlist;
    uint32_t *tag_inserted;

    MESH_OS_LAYER_DLIST_ENTRY *entry_inlist;
    MESH_OS_LAYER_DLIST_ENTRY *entry_inserted;

    if (inlist == NULL || inserted == NULL || list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_after fail, list = %p, inlist = %p, inserted = %p\n",
              list, inlist, inserted);
        return;
    }

    tag_inlist = (uint32_t *)((uint8_t *)inlist  - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, tag));
    tag_inserted = (uint32_t *)((uint8_t *)inserted  - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, tag));

    if (*tag_inlist == 0x20111118 && *tag_inserted == 0x20111118) { //valid entry
        entry_inlist = (MESH_OS_LAYER_DLIST_ENTRY *)((uint8_t *)inlist - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));
        entry_inserted = (MESH_OS_LAYER_DLIST_ENTRY *)((uint8_t *)inserted - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));
        LIST_INSERT_AFTER(entry_inlist, entry_inserted, entry);
        ((MESH_OS_LAYER_DLIST *)list)->count++;
    }
}

void bt_mesh_os_layer_ds_dlist_insert_before(void *list, void *inlist, void *inserted)
{
    uint32_t *tag_inlist;
    uint32_t *tag_inserted;

    MESH_OS_LAYER_DLIST_ENTRY *entry_inlist;
    MESH_OS_LAYER_DLIST_ENTRY *entry_inserted;

    if (inlist == NULL || inserted == NULL || list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_before fail, list = %p, inlist = %p, inserted = %p\n",
              list, inlist, inserted);
        return;
    }

    tag_inlist = (uint32_t *)((uint8_t *)inlist  - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, tag));
    tag_inserted = (uint32_t *)((uint8_t *)inserted  - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, tag));

    if (*tag_inlist == 0x20111118 && *tag_inserted == 0x20111118) { //valid entry
        entry_inlist = (MESH_OS_LAYER_DLIST_ENTRY *)((uint8_t *)inlist - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));
        entry_inserted = (MESH_OS_LAYER_DLIST_ENTRY *)((uint8_t *)inserted - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));
        LIST_INSERT_BEFORE(entry_inlist, entry_inserted, entry);
        ((MESH_OS_LAYER_DLIST *)list)->count++;
    }
}

void *bt_mesh_os_layer_ds_dlist_first(void *list)
{
    void *pdat = NULL;

    if (list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_first fail, list(NULL)\n");
        return NULL;
    }

    if (list != NULL && LIST_FIRST(&((MESH_OS_LAYER_DLIST *)list)->head)) {
        pdat = ((uint8_t *)LIST_FIRST(&((MESH_OS_LAYER_DLIST *)list)->head) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));
    }

    return pdat;
}

void *bt_mesh_os_layer_ds_dlist_next(void *list, void *dat)
{
    uint32_t *ptag;
    void *pdat = NULL;

    MESH_OS_LAYER_DLIST_ENTRY *pentry;

    if (list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_next fail, list(NULL)\n");
        return NULL;
    }

    ptag = (uint32_t *)((uint8_t *)dat  - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data) + offsetof(MESH_OS_LAYER_DLIST_ENTRY, tag));
    pentry = (MESH_OS_LAYER_DLIST_ENTRY *)((uint8_t *)dat - offsetof(MESH_OS_LAYER_DLIST_ENTRY, data));

    if (*ptag == 0x20111118) { //valid entry
        pdat = LIST_NEXT(pentry, entry);
        if (pdat != NULL) {
            pdat += offsetof(MESH_OS_LAYER_DLIST_ENTRY, data);
        }
    }

    return pdat;
}

uint32_t bt_mesh_os_layer_ds_dlist_count(void *list)
{
    if (list == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_dlist_count fail, list(NULL)\n");
        return 0;
    }

    if (((MESH_OS_LAYER_DLIST *)list)->tag == 0x19760108) {
        return ((MESH_OS_LAYER_DLIST *)list)->count;
    }

    return 0;
}

/*******************************************
  ****************List operation***************
********************************************/

////////////////////////////////////////////////////////////////////////////////
// Public Functions ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void *bt_mesh_os_layer_ds_queue_alloc(void)
{
    MESH_OS_LAYER_QUEUE *queue;

    queue = (MESH_OS_LAYER_QUEUE *)bt_mesh_os_layer_memory_alloc(sizeof(MESH_OS_LAYER_QUEUE));
    if (!queue) {
        LOG_E(mesh_os_layer, "mesh_os_queue_alloc fail\n");
        return NULL;
    }

    queue->tag = 0x19760108;
    queue->count = 0;
    queue->head.stqh_first = NULL;
    queue->head.stqh_last = &(queue->head.stqh_first);

    return queue;
}

void bt_mesh_os_layer_ds_queue_free(void *queue)
{
    void *dat;

    if (queue == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_free fail, queue(NULL)\n");
        return;
    }

    while ((dat = bt_mesh_os_layer_ds_queue_pop(queue))) {

        bt_mesh_os_layer_ds_queue_entry_free(dat);
    }

    bt_mesh_os_layer_memory_free(queue);
}

void *bt_mesh_os_layer_ds_queue_entry_alloc(uint32_t size)
{
    MESH_OS_LAYER_QUEUE_ENTRY *new_entry;

    if (size == 0) {
        LOG_E(mesh_os_layer, "mesh_os_queue_entry_alloc fail, size(0)\n");
        return NULL;
    }

    new_entry = (MESH_OS_LAYER_QUEUE_ENTRY *) bt_mesh_os_layer_memory_alloc(sizeof(MESH_OS_LAYER_QUEUE_ENTRY) + size);

    if (new_entry) {
        new_entry->tag = 0x20111118;
        return (uint8_t *)new_entry + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data);
    }

    LOG_I(mesh_os_layer, "mesh_os_queue_entry_alloc fail\n");
    return NULL;
}

void bt_mesh_os_layer_ds_queue_entry_free(void *dat)
{
    uint32_t *ptag;
    MESH_OS_LAYER_QUEUE_ENTRY *pentry;

    if (dat == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_entry_free fail, dat(NULL)\n");
        return;
    }

    ptag = (uint32_t *)((uint8_t *)dat  - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, tag));
    pentry = (MESH_OS_LAYER_QUEUE_ENTRY *)((uint8_t *)dat - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));

    if (*ptag == 0x20111118) { //valid entry
        bt_mesh_os_layer_memory_free((uint8_t *)pentry);
    }
}

void bt_mesh_os_layer_ds_queue_push(void *queue, void *dat)
{
    uint32_t *ptag;

    MESH_OS_LAYER_QUEUE_ENTRY *pentry;

    if (queue == NULL || dat == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_push fail, queue = %p, dat = %p\n", queue, dat);
        return;
    }

    if (((MESH_OS_LAYER_QUEUE *)queue)->tag == 0x19760108) {
        ptag = (uint32_t *)((uint8_t *)dat  - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, tag));
        pentry = (MESH_OS_LAYER_QUEUE_ENTRY *)((uint8_t *)dat - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));

        if (*ptag == 0x20111118) { //valid entry
            STAILQ_INSERT_TAIL(&((MESH_OS_LAYER_QUEUE *)queue)->head, pentry, entry);
            ((MESH_OS_LAYER_QUEUE *)queue)->count++;
        }
    }
}

void bt_mesh_os_layer_ds_queue_push_front(void *queue, void *dat)
{
    uint32_t *ptag;

    MESH_OS_LAYER_QUEUE_ENTRY *pentry;

    if (queue == NULL || dat == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_front fail, queue = %p, dat = %p\n", queue, dat);
        return;
    }

    if (((MESH_OS_LAYER_QUEUE *)queue)->tag == 0x19760108) {
        ptag = (uint32_t *)((uint8_t *)dat  - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, tag));
        pentry = (MESH_OS_LAYER_QUEUE_ENTRY *)((uint8_t *)dat - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));

        if (*ptag == 0x20111118) { //valid entry
            STAILQ_INSERT_HEAD(&((MESH_OS_LAYER_QUEUE *)queue)->head, pentry, entry);
            ((MESH_OS_LAYER_QUEUE *)queue)->count++;
        }
    }
}

void *bt_mesh_os_layer_ds_queue_pop(void *queue)
{
    void *pdat = NULL;

    if (queue == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_pop, queue(NULL)\n");
        return NULL;
    }

    if (STAILQ_FIRST(&((MESH_OS_LAYER_QUEUE *)queue)->head)) {
        pdat = ((uint8_t *)STAILQ_FIRST(&((MESH_OS_LAYER_QUEUE *)queue)->head) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));
        STAILQ_REMOVE_HEAD(&((MESH_OS_LAYER_QUEUE *)queue)->head, entry);
        ((MESH_OS_LAYER_QUEUE *)queue)->count--;
    }

    return pdat;
}

void *bt_mesh_os_layer_ds_queue_first(void *queue)
{
    void *pdat = NULL;

    if (queue == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_first, queue(NULL)\n");
        return NULL;
    }

    if (STAILQ_FIRST(&((MESH_OS_LAYER_QUEUE *)queue)->head)) {
        pdat = ((uint8_t *)STAILQ_FIRST(&((MESH_OS_LAYER_QUEUE *)queue)->head) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));
    }

    return pdat;
}

void *bt_mesh_os_layer_ds_queue_next(void *queue, void *dat)
{
    uint32_t *ptag;
    void *pdat = NULL;

    MESH_OS_LAYER_QUEUE_ENTRY *pentry;

    if (queue == NULL || dat == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_next fail, queue = %p, dat = %p\n", queue, dat);
        return NULL;
    }

    ptag = (uint32_t *)((uint8_t *)dat  - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, tag));
    pentry = (MESH_OS_LAYER_QUEUE_ENTRY *)((uint8_t *)dat - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));

    if (*ptag == 0x20111118) { //valid entry
        if (STAILQ_NEXT(pentry, entry)) {
            pdat = ((uint8_t *)STAILQ_NEXT(pentry, entry) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));
        }
    }

    return pdat;
}

void bt_mesh_os_layer_ds_queue_insert_after(void *queue, void *inqueue, void *inserted)
{
    uint32_t *tag_inqueue;
    uint32_t *tag_inserted;

    MESH_OS_LAYER_QUEUE_ENTRY *entry_inqueue;
    MESH_OS_LAYER_QUEUE_ENTRY *entry_inserted;

    if (queue == NULL || inqueue == NULL || inserted == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_after fail, queue = %p, inqueue = %p, inserted = %p\n",
              queue, inqueue, inserted);
        return;
    }

    tag_inqueue = (uint32_t *)((uint8_t *)inqueue  - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, tag));
    tag_inserted = (uint32_t *)((uint8_t *)inserted  - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, tag));

    if (*tag_inqueue == 0x20111118 && *tag_inserted == 0x20111118) { //valid entry
        entry_inqueue = (MESH_OS_LAYER_QUEUE_ENTRY *)((uint8_t *)inqueue - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));
        entry_inserted = (MESH_OS_LAYER_QUEUE_ENTRY *)((uint8_t *)inserted - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));
        STAILQ_INSERT_AFTER(&((MESH_OS_LAYER_QUEUE *)queue)->head, entry_inqueue, entry_inserted, entry);
        ((MESH_OS_LAYER_QUEUE *)queue)->count++;
    }

}

void bt_mesh_os_layer_ds_queue_remove(void *queue, void *removed)
{
    uint32_t *tag_removed;

    MESH_OS_LAYER_QUEUE_ENTRY *entry_removed;

    if (queue == NULL || removed == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_remove, queue = %p, removed = %p\n", queue, removed);
        return;
    }

    tag_removed = (uint32_t *)((uint8_t *)removed - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data) + offsetof(MESH_OS_LAYER_QUEUE_ENTRY, tag));

    if (*tag_removed == 0x20111118) { //valid entry
        entry_removed = (MESH_OS_LAYER_QUEUE_ENTRY *)((uint8_t *)removed - offsetof(MESH_OS_LAYER_QUEUE_ENTRY, data));
        STAILQ_REMOVE(&((MESH_OS_LAYER_QUEUE *)queue)->head, entry_removed, queue_entry, entry);
        ((MESH_OS_LAYER_QUEUE *)queue)->count--;
    }
}

uint32_t bt_mesh_os_layer_ds_queue_count(void *queue)
{
    if (queue == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_queue_count, queue(NULL)\n");
        return 0;
    }

    if (((MESH_OS_LAYER_QUEUE *)queue)->tag == 0x19760108) {
        return ((MESH_OS_LAYER_QUEUE *)queue)->count;
    }

    return 0;
}

/*******************************************
  ****************Cryption tool ***************
********************************************/
#include "hal_aes.h"
#include "mbedtls/md5.h"
#if !defined(__GNUC__) // || defined(MTK_MT7933_BT_ENABLE) // If you want to use SW AES, unmark it.
#include "mbedtls/aes.h"
#endif /* #if !defined(__GNUC__) // || defined(MTK_MT7933_BT_ENABLE) // If you want to use SW AES, unmark it. */
#ifdef MTK_TFM_ENABLE
#include "tfm_platform_api.h"
#endif /* #ifdef MTK_TFM_ENABLE */

#include "memory_attribute.h"

ATTR_RWDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN uint8_t enc_temp_buf[32] = {0};
ATTR_RWDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN uint8_t plain_temp_buf[16] = {0};
ATTR_RWDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN uint8_t key_temp_buf[16] = {0};

ATTR_RWDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN hal_aes_buffer_t enc_temp = {
    .buffer = enc_temp_buf,
    .length = sizeof(enc_temp_buf)
};
ATTR_RWDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN hal_aes_buffer_t plain_temp = {
    .buffer = plain_temp_buf,
    .length = sizeof(plain_temp_buf)
};
ATTR_RWDATA_IN_NONCACHED_SYSRAM_4BYTE_ALIGN hal_aes_buffer_t key_temp = {
    .buffer = key_temp_buf,
    .length = sizeof(key_temp_buf)
};

static void bt_mesh_os_layer_byte_reverse_copy(uint8_t *dst, const uint8_t *src, uint32_t length)
{
    uint32_t i;

    if (dst == NULL || src == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_byte_copy fail, src = %p, dst = %p\n", src, dst);
        return;
    }

    for (i = 0; i < length; i++) {
        dst[i] = src[length - i - 1];
    }
}

static void bt_mesh_os_layer_byte_reverse(uint8_t *src, uint32_t length)
{
    uint32_t i;
    uint8_t temp;

    if (src == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_byte_reverse fail, src(NULL)\n");
        return;
    }

    for (i = 0; i < length / 2; i++) {
        temp = src[i];
        src[i] = src[length - i - 1];
        src[length - i - 1] = temp;
    }
}


extern void bt_os_take_stack_mutex(void);
extern void bt_os_give_stack_mutex(void);

void bt_mesh_os_layer_aes_encrypt(uint8_t *encrypted_data, uint8_t *plain_text, uint8_t *key)
{
    if (encrypted_data == NULL || plain_text == NULL || key == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_aes_encrypt fail, encrypted_data = %p, plain_text = %p, key = %p\n",
              encrypted_data, plain_text, key);
        return;
    }

#if defined(__GNUC__) // && !defined(MTK_MT7933_BT_ENABLE) // If you want to use SW AES, unmark it.
#if 0
    uint8_t enc_temp_buf[32] = {0};
    uint8_t plain_temp_buf[16] = {0};
    uint8_t key_temp_buf[16] = {0};

    bt_mesh_os_layer_aes_buffer_t enc_temp = {
        .buffer = (uint8_t *)(&enc_temp_buf),
        .length = 32,
    };
    bt_mesh_os_layer_aes_buffer_t plain_temp = {
        .buffer = (uint8_t *)(&plain_temp_buf),
        .length = 16,
    };
    bt_mesh_os_layer_aes_buffer_t key_temp = {
        .buffer = (uint8_t *)(&key_temp_buf),
        .length = 16,
    };
#else /* #if 0 */
    memset(enc_temp.buffer, 0, sizeof(enc_temp_buf));
    memset(plain_temp.buffer, 0, sizeof(plain_temp_buf));
    memset(key_temp.buffer, 0, sizeof(key_temp_buf));
#endif /* #if 0 */
    //LOG_I(mesh_os_layer,"bt_mesh_os_layer_aes_encrypt use hal version");
#if 0
    LOG_I(mesh_os_layer, "plain_text: ");
    for (int i = 0; i < 16; i++) {
        LOG_I(mesh_os_layer, "%02x ", plain_text[0]);
    }
    LOG_I(mesh_os_layer, "\nkey: ");
    for (int i = 0; i < 16; i++) {
        LOG_I(mesh_os_layer, "%02x ", key[0]);
    }
    LOG_I(mesh_os_layer, "\n");
#endif /* #if 0 */
    memcpy(plain_temp.buffer, plain_text, 16);
    memcpy(key_temp.buffer, key, 16);
    bt_os_take_stack_mutex();
#ifndef MTK_TFM_ENABLE
    hal_aes_status_t status = hal_aes_ecb_encrypt((hal_aes_buffer_t *)&enc_temp, (hal_aes_buffer_t *)&plain_temp, (hal_aes_buffer_t *)&key_temp);
    bt_os_give_stack_mutex();
    if (status != HAL_AES_STATUS_OK) {
        LOG_E(mesh_os_layer, " mesh hal_aes_ecb_encrypt: error(%d)", status);
    }
#else /* #ifndef MTK_TFM_ENABLE */
    int32_t ret = tfm_aes_ecb_encrypt((void *)&enc_temp, (void *)&plain_temp, (void *)&key_temp);
    bt_os_give_stack_mutex();
    if (ret != 0) {
        LOG_E(mesh_os_layer, " mesh hal_aes_ecb_encrypt: error(%d)", ret);
    }
#endif /* #ifndef MTK_TFM_ENABLE */
    //LOG_I(mesh_os_layer,"enc_len =%d, plain_text_len temp = %d\n", enc_temp.length, plain_temp.length);
#if 0
    LOG_I(mesh_os_layer, "encrypted_data: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x ", enc_temp.buffer[0]);
    }
    printf("\n");
#endif /* #if 0 */
    memcpy(encrypted_data, enc_temp.buffer, 16);
#else /* #if defined(__GNUC__) // && !defined(MTK_MT7933_BT_ENABLE) // If you want to use SW AES, unmark it. */
    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_enc(&context, key, sizeof(bt_key_t) * 8);
    mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, (unsigned char *)plain_text, (unsigned char *)encrypted_data);
    mbedtls_aes_free(&context);
    //LOG_I(mesh_os_layer,"bt_mesh_os_layer_aes_encrypt use mbed version");
#endif /* #if defined(__GNUC__) // && !defined(MTK_MT7933_BT_ENABLE) // If you want to use SW AES, unmark it. */
}

#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdh.h"

mbedtls_ecdh_context ctx_cli = {0};
mbedtls_ctr_drbg_context ctr_drbg = {0};
static uint8_t p256_runned = 0;
uint8_t buffer[1024] = {0};
size_t olen;
uint8_t ecdh_buf_byte_1 = 0x00;
#define ECDH_PUBLIC_KEY_LEN 64
#define ECDH_SECRET_LEN     32
static uint32_t p256_latency = 0;

void bt_mesh_os_layer_read_local_p256_public_key(uint8_t *public_key)
{

    int ret;
    mbedtls_entropy_context entropy;
    const char pers[] = "ecdh";

    if (public_key == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_read_P256_key fail, public_key(NULL)\n");
        return;
    }

    //    ((void) argc);
    //  ((void) argv);
    p256_latency = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (p256_runned == 1) {
        mbedtls_ecdh_free(&ctx_cli);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        p256_runned = 0;
    }


    mbedtls_ecdh_init(&ctx_cli);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    p256_runned = 1;

    /*
     * Initialize random number generation
     */
    LOG_I(mesh_os_layer, "Seeding the random number generator...\n");

    mbedtls_entropy_init(&entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     sizeof pers)) != 0) {
        LOG_E(mesh_os_layer, "mbedtls_ctr_drbg_seed fail, returned %d\n", ret);
        goto exit;
    }

    LOG_I(mesh_os_layer, " ok\n");

    /*
     * Client: inialize context and generate keypair
     */
    LOG_I(mesh_os_layer, "  . Setting up client context...\n");

    ret = mbedtls_ecp_group_load(&ctx_cli.MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        LOG_E(mesh_os_layer, "mbedtls_ecp_group_load fail, returned %d\n", ret);
        goto exit;
    }

    ret = mbedtls_ecdh_make_public(&ctx_cli, &olen, buffer, sizeof(buffer),
                                   mbedtls_ctr_drbg_random, &ctr_drbg);
    LOG_I(mesh_os_layer, "mbedtls_ecdh_make_public: len = %d\n", olen);
    if (ret != 0) {
        LOG_E(mesh_os_layer, "mbedtls_ecdh_gen_public fail, returned %d\n", ret);
        goto exit;
    }

    ret = mbedtls_ecp_point_write_binary(&ctx_cli.MBEDTLS_PRIVATE(grp), &ctx_cli.MBEDTLS_PRIVATE(Q), MBEDTLS_ECP_PF_UNCOMPRESSED, &olen, (uint8_t *)(&buffer), sizeof(buffer));
    if (ret != 0) {
        LOG_E(mesh_os_layer, "mbedtls_ecp_point_write_binary fail, returned %d\n", ret);
        goto exit;
    }
    LOG_I(mesh_os_layer, "mbedtls_ecp_point_write_binary:olen = %d\n", olen);
    configASSERT(olen == (ECDH_PUBLIC_KEY_LEN + 1));
    LOG_I(mesh_os_layer, "buffer: \n");
    for (int i = 0; i < ECDH_PUBLIC_KEY_LEN + 1; i++) {
        LOG_I(mesh_os_layer, "%02x ", buffer[i]);
    }
    LOG_I(mesh_os_layer, "\n");
    ecdh_buf_byte_1 = buffer[0]; //Tengfei: big endian
    //memcpy(public_key, &(buffer[1]), ECDH_PUBLIC_KEY_LEN);

    bt_mesh_os_layer_byte_reverse_copy(public_key, &(buffer[1]), 32);

    bt_mesh_os_layer_byte_reverse_copy(public_key + 32,  &(buffer[33]), 32);
    memset(buffer, 0, sizeof(buffer));
    mbedtls_entropy_free(&entropy);
    LOG_W(mesh_os_layer, "read public latency (%d)ms", xTaskGetTickCount() * portTICK_PERIOD_MS - p256_latency);
    LOG_I(mesh_os_layer, " ok\n");
    return;

exit:

    mbedtls_ecdh_free(&ctx_cli);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    LOG_I(mesh_os_layer, " failed\n");
    configASSERT(0);
    return;
}

void bt_mesh_os_layer_generate_dhkey(uint8_t *peer_pb_key, uint8_t *dh_key)
{

    int ret;

    if (peer_pb_key == NULL || dh_key == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_generate_dhkey fail, peer_pb_key = %p, dh_key = %p\n", peer_pb_key, dh_key);
        return;
    }

    p256_latency = xTaskGetTickCount() * portTICK_PERIOD_MS;

    /*
     * Client: read peer's key and generate shared secret
     */
    LOG_I(mesh_os_layer, "  . Client reading server key and computing secret...\n");
    LOG_I(mesh_os_layer, "bt_mesh_os_layer_generate_dhkey:\n ecdh byte 1 = %02x\n", ecdh_buf_byte_1);
    LOG_I(mesh_os_layer, "peer_pb_key: \n");
    for (int i = 0; i < ECDH_PUBLIC_KEY_LEN; i++) {
        LOG_I(mesh_os_layer, "%02x ", peer_pb_key[i]);
    }
    LOG_I(mesh_os_layer, "\n");

    buffer[0] = ecdh_buf_byte_1;
    memcpy(&(buffer[1]), peer_pb_key, ECDH_PUBLIC_KEY_LEN);

    bt_mesh_os_layer_byte_reverse_copy(&(buffer[1]), peer_pb_key, 32);
    bt_mesh_os_layer_byte_reverse_copy(&(buffer[33]), peer_pb_key + 32, 32);

    ret = mbedtls_ecp_point_read_binary(&ctx_cli.MBEDTLS_PRIVATE(grp), &ctx_cli.MBEDTLS_PRIVATE(Qp), (uint8_t *)(&buffer), ECDH_PUBLIC_KEY_LEN + 1);
    if (ret != 0) {
        LOG_E(mesh_os_layer, "mbedtls_ecp_point_read_binary fail, returned %d\n", ret);
        goto exit;
    }

    //p256_latency = xTaskGetTickCount() * portTICK_PERIOD_MS;
    ret = mbedtls_ecdh_calc_secret(&ctx_cli, &olen, buffer, sizeof(buffer),
                                   mbedtls_ctr_drbg_random, &ctr_drbg);

    //LOG_W(mesh_os_layer, "cal ecdh latency (%d)ms", xTaskGetTickCount() * portTICK_PERIOD_MS - p256_latency);
    if (ret != 0) {
        LOG_E(mesh_os_layer, "mbedtls_ecdh_calc_secret fail, returned %d\n", ret);
        goto exit;
    }
    configASSERT(olen == ECDH_SECRET_LEN);
    //mbedtls_mpi_write_binary( &ctx_cli.z, dh_key, ECDH_SECRET_LEN );
    bt_mesh_os_layer_byte_reverse_copy(dh_key, buffer, ECDH_SECRET_LEN); // convert to little endian
    LOG_I(mesh_os_layer, "bt_mesh_os_layer_generate_dhkey:");
    for (int i = 0; i < ECDH_SECRET_LEN; i++) {
        LOG_I(mesh_os_layer, "%02x ", dh_key[i]);
    }
    LOG_I(mesh_os_layer, "\n");
    LOG_I(mesh_os_layer, " ok\n");

    LOG_W(mesh_os_layer, "cal ecdh latency (%d)ms", xTaskGetTickCount() * portTICK_PERIOD_MS - p256_latency);
    return;

exit:
    LOG_I(mesh_os_layer, " failed\n");
    configASSERT(0);
    return;
}

int bt_mesh_os_layer_validate_public_key(uint8_t *public_key)
{
    if (public_key == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_validate_key, public_key(NULL)\n");
        return -1;
    }

#ifndef TT_RM_ECC
    /*input "public_key" is little endian*/
    uECC_Curve curve = uECC_secp256r1();
#if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0
    bt_mesh_os_layer_byte_reverse(public_key, 32);
    bt_mesh_os_layer_byte_reverse(public_key + 32, 32);
#endif /* #if uECC_VLI_NATIVE_LITTLE_ENDIAN == 0 */

    return uECC_valid_public_key(public_key, curve);
#else /* #ifndef TT_RM_ECC */
    return -1;
#endif /* #ifndef TT_RM_ECC */
}

//timer
static TimerHandle_t mesh_os_timer = NULL; /**< Timer handler. */
static bt_mesh_os_layer_timer_expired_t bt_rtos_timer_cb;  /**< Timer callback function. */

void bt_mesh_os_layer_register_timer_callback(bt_mesh_os_layer_timer_expired_t callback)
{
    bt_rtos_timer_cb = callback;
}

static void bt_mesh_os_layer_rtos_timer_os_expire(TimerHandle_t timer)
{
    if (bt_rtos_timer_cb != NULL) {
        bt_rtos_timer_cb();
    }
}

void bt_mesh_os_layer_init_timer(void)
{
    if (mesh_os_timer == NULL) {
        mesh_os_timer = xTimerCreate("mesh timer", 0xffff, pdFALSE, NULL, bt_mesh_os_layer_rtos_timer_os_expire);
    }
    configASSERT(mesh_os_timer != NULL);
}

void bt_mesh_os_layer_deinit_timer(void)
{
    BaseType_t ret;

    if (mesh_os_timer != NULL) {
        ret = xTimerDelete(mesh_os_timer, 0);
        if (ret != pdPASS) {
            LOG_I(mesh_os_layer, "%s failed, ret = %d\n", __func__, ret);
            configASSERT(0);
        }
        mesh_os_timer = NULL;
    }
}

uint32_t bt_mesh_os_layer_create_semaphore()
{
    return (uint32_t)xSemaphoreCreateBinary();
}

void bt_mesh_os_layer_take_semaphore(uint32_t semaphore_id)
{
    if (xSemaphoreTake((SemaphoreHandle_t)semaphore_id, portMAX_DELAY) == pdFALSE) {
        LOG_D(mesh_os_layer, "sema_id take failed\n");
    }
}

void bt_mesh_os_layer_give_semaphore(uint32_t semaphore_id)
{
    xSemaphoreGive((SemaphoreHandle_t)semaphore_id);
}

void bt_mesh_os_layer_delete_semaphore(uint32_t semaphore_id)
{
    vSemaphoreDelete((SemaphoreHandle_t)semaphore_id);
}

uint32_t bt_mesh_os_layer_timer_get_tickcount(void)
{
    return xTaskGetTickCount();
}

uint32_t bt_mesh_os_layer_ms_to_tick(uint32_t time_ms)
{
    return (time_ms / portTICK_PERIOD_MS);
}

uint32_t bt_mesh_os_layer_is_timer_active(void)
{
    configASSERT(mesh_os_timer != NULL);
    if ((mesh_os_timer != NULL) && (xTimerIsTimerActive(mesh_os_timer) != pdFALSE)) {
        return 1;
    } else {
        return 0;
    }
}

void bt_mesh_os_layer_start_timer(uint32_t tick)
{
    configASSERT(mesh_os_timer != NULL);
    //configASSERT(tick > 0);
    configASSERT(pdFAIL != xTimerChangePeriod(mesh_os_timer, tick + 1, 0));
}

void bt_mesh_os_layer_stop_timer(void)
{
    configASSERT(mesh_os_timer != NULL);
    configASSERT(pdFAIL != xTimerStop(mesh_os_timer, 0));
}

void bt_mesh_os_layer_init_random_seed()
{
    uint32_t random_seed;
#ifndef MTK_TFM_ENABLE
    hal_trng_status_t ret = HAL_TRNG_STATUS_OK;

    ret = hal_trng_init();
    if (HAL_TRNG_STATUS_OK != ret) {
        LOG_E(mesh_os_layer, "mesh generate_random_address--error 1\n");
    }

    ret = hal_trng_get_generated_random_number(&random_seed);
    if (HAL_TRNG_STATUS_OK != ret) {
        LOG_E(mesh_os_layer, "mesh generate_random_address--error 2\n");
    }

    hal_trng_deinit();
#else /* #ifndef MTK_TFM_ENABLE */
    int ret;

    ret = tfm_generate_random(&ctr_drbg, (unsigned char *)&random_seed, sizeof(random_seed));
    if (ret != 0) {
        LOG_E(mesh_os_layer, "mesh generate_random_address--error 1\n");
    }
#endif /* #ifndef MTK_TFM_ENABLE */

    srand(random_seed);
}

void bt_mesh_os_layer_get_random(uint8_t *rand_buf, uint8_t len)
{
    uint32_t random_seed, i, j;

    if (rand_buf == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_get_random fail, rand_buf(NULL)\n");
        return;
    }

    for (i = 0; i < (len / 4); i++) {
        random_seed = rand();
        rand_buf[4 * i + 0] = random_seed & 0xFF;
        rand_buf[4 * i + 1] = (random_seed >> 8) & 0xFF;
        rand_buf[4 * i + 2] = (random_seed >> 16) & 0xFF;
        rand_buf[4 * i + 3] = (random_seed >> 24) & 0xFF;

    }

    j = len % 4;
    if (j > 0) {
        random_seed = rand();
        while (j > 0) {
            rand_buf[4 * i + (j - 1)] = random_seed >> (8 * (j - 1)) & 0xFF;
            j--;
        }
    }

    return;
}

void bt_mesh_os_layer_disable_interrupt(uint32_t *nvic_mask)
{
    if (nvic_mask == NULL) {
        LOG_E(mesh_os_layer, "mesh_osr_disable_interrupt fail, nvic_mask(NULL)\n");
        return;
    }

    hal_nvic_save_and_set_interrupt_mask(nvic_mask);
}

void bt_mesh_os_layer_enable_interrupt(uint32_t nvic_mask)
{
    hal_nvic_restore_interrupt_mask(nvic_mask);
}

void bt_mesh_os_layer_trigger_interrupt(uint32_t is_from_isr)
{
    bt_trigger_interrupt(is_from_isr);
}

void bt_mesh_os_layer_enter_critical(void)
{
    MESH_MUTEX_LOCK();
}

void bt_mesh_os_layer_exit_critical(void)
{
    MESH_MUTEX_UNLOCK();
}

uint32_t bt_mesh_os_layer_create_mutex(void)
{
    return (uint32_t)xSemaphoreCreateRecursiveMutex();
}

void bt_mesh_os_layer_delete_mutex(uint32_t mutex_handle)
{
    vSemaphoreDelete((SemaphoreHandle_t)mutex_handle);
}

void bt_mesh_os_layer_take_mutex(uint32_t mutex_handle)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        LOG_E(mesh_os_layer, "mesh mutex take fail (scheduler not started)");
        return;
    }
    if (!mutex_handle) {
        LOG_E(mesh_os_layer, "mutex handle is null");
        return;
    }
    xSemaphoreTakeRecursive((SemaphoreHandle_t)mutex_handle, portMAX_DELAY);
}

void bt_mesh_os_layer_give_mutex(uint32_t mutex_handle)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        LOG_E(mesh_os_layer, "mesh mutex get fail (scheduler not started)");
        return;
    }
    if (!mutex_handle) {
        LOG_E(mesh_os_layer, "mutex handle is null");
        return;
    }
    xSemaphoreGiveRecursive((SemaphoreHandle_t)mutex_handle);
}

void bt_mesh_os_layer_deregister_callback(void *callback)
{
    if (callback == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_deregister_callback fail, callback(NULL)\n");
        return;
    }

    bt_callback_manager_deregister_callback(bt_callback_type_app_event, (void *)callback);
}
void bt_mesh_os_layer_register_callback(void *callback)
{
    if (callback == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_register_callback fail, callback(NULL)\n");
        return;
    }

    bt_callback_manager_register_callback(bt_callback_type_app_event, MODULE_MASK_SYSTEM | MODULE_MASK_GAP | MODULE_MASK_GATT, (void *)callback);
}

#ifdef BT_MESH_NVDM_NONBLOCK
void bt_mesh_os_nvdm_cbk(nvdm_status_t status, void *user_data)
{
    if (status != NVDM_STATUS_OK) {
        LOG_E(mesh_os_layer, "%s, mesh_nvdm ERR(%d)", user_data, status);
    }
}
#define MESH_OS_NVDM_WRITE_DATE_ITEM(...) nvdm_write_data_item_non_blocking(__VA_ARGS__,(bt_mesh_os_nvdm_cbk),__FUNCTION__)
#else /* #ifdef BT_MESH_NVDM_NONBLOCK */
#define MESH_OS_NVDM_WRITE_DATE_ITEM(...) nvdm_write_data_item(__VA_ARGS__)
#endif /* #ifdef BT_MESH_NVDM_NONBLOCK */

#ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM
static record_node_t *gp_list_tail;
static record_node_t *gp_list_head;
static seq_num_record_block_t g_seq_num_blk;
uint32_t g_bt_mesh_record_mutex;

static record_node_t *_bt_mesh_record_search_node_list(const char *id)
{
    if (!id) {
        return NULL;
    }

    record_node_t *tmp_node = gp_list_head;
    while (tmp_node != NULL) {
        if (!memcmp(id, tmp_node->item.record_id, tmp_node->item.id_len)) {
            break;
        }
        tmp_node = tmp_node->next;
    }
    return tmp_node;
}

static bool _bt_mesh_record_update_node_data(record_node_t *node, uint8_t *data, uint32_t data_len)
{
    if (!node || !data || !data_len)
        return false;

    if (node->item.data == NULL) {
        node->item.data = bt_mesh_os_layer_memory_alloc(data_len);
        if (!node->item.data) {
            LOG_W(mesh_os_layer, "Fail to alloc cache space.");
            return false;
        }
    } else if (node->item.data_len != data_len) {
        LOG_E(mesh_os_layer, "Why data len mismatch?");
        return false;
    }

    memcpy(node->item.data, data, data_len);
    node->item.data_len = data_len;
    return true;
}


static record_node_t *_bt_mesh_record_add_new_node_to_tail(const char *id, uint8_t *data, uint32_t data_len)
{
    if (!id) {
        return NULL;
    }

    record_node_t *new_node;
    uint8_t *p_buffer;
    uint32_t _id_len = strlen(id) + 1;
    uint32_t alloc_size = sizeof(record_node_t) + _id_len;
    bool ret;

    p_buffer = (void *)bt_mesh_os_layer_memory_alloc(alloc_size);
    if (p_buffer) {
        memset(p_buffer, 0, alloc_size);
        new_node = (record_node_t *)p_buffer;
        p_buffer += sizeof(record_node_t);
        new_node->item.record_id = (char *)p_buffer;
        new_node->item.id_len = _id_len;
        memcpy(new_node->item.record_id, id, _id_len);

        if (data != NULL && data_len != 0) {
            ret = _bt_mesh_record_update_node_data(new_node, data, data_len);
            if (!ret) {
                LOG_W(mesh_os_layer, "Fail to add node data!");
                return new_node;
            }
        }

        if (gp_list_head == NULL) {
            gp_list_tail = new_node;
            gp_list_head = gp_list_tail;
        } else {
            gp_list_tail->next = new_node;
            gp_list_tail = new_node;
        }

        return new_node;
    } else {
        LOG_W(mesh_os_layer, "Fail to alloc new cache node.");
    }

    return NULL;
}



static bool _bt_mesh_record_update_node_by_id(const char *id, uint8_t *data, uint32_t data_len)
{
    bool ret = false;

    if (!id || !data_len || !data) {
        //LOG_E(mesh_os_layer, "parameter Error! id:%d, p_data:%p, dataSize:%d",id, data, data_len);
        return false;
    }
    record_node_t *p_node;
    p_node = _bt_mesh_record_search_node_list(id);
    if (p_node) {
        ret = _bt_mesh_record_update_node_data(p_node, data, data_len);
    } else {
        p_node = _bt_mesh_record_add_new_node_to_tail(id, data, data_len);
        if (p_node)
            ret = true;
    }

    return ret;
}

/*Reserved API. No one is using now*/
bool _bt_mesh_record_delete_node(const char *id)
{
    if (!id)
        return true;

    record_node_t *current_node = gp_list_head;
    record_node_t *previous_node = gp_list_head;

    while (current_node != NULL) {
        if (!memcmp(id, current_node->item.record_id, current_node->item.id_len)) {
            LOG_I(mesh_os_layer, "Found target! deleting");
            if (current_node == gp_list_head) {
                //we need assign the second node to gp_list_head, if head node is deleted in this list.
                gp_list_head = current_node->next;
                previous_node = gp_list_head; //Change prev to new head for setting tail node
            } else {
                previous_node->next = current_node->next;
            }

            if (current_node == gp_list_tail) {
                //update tail node if we deleted at tail
                gp_list_tail = previous_node;
            }

            if (current_node->item.data)
                bt_mesh_os_layer_memory_free(current_node->item.data);
            bt_mesh_os_layer_memory_free((uint8_t *)current_node);
            break;
        }
        previous_node = current_node;
        current_node = current_node->next;
    }

    return true;
}

static void _bt_mesh_record_free_node_list(void)
{
    record_node_t *current_node = gp_list_head;
    record_node_t *next_node;

    while (current_node != NULL) {
        next_node = current_node->next;
        if (current_node->item.data)
            bt_mesh_os_layer_memory_free(current_node->item.data);
        bt_mesh_os_layer_memory_free((uint8_t *)current_node);
        current_node = next_node;
    }

    gp_list_head = NULL;
    gp_list_tail = NULL;
}

/*mesh record mutex can always exist for using*/
static uint32_t _bt_mesh_os_get_record_mutex(void)
{
    if (!g_bt_mesh_record_mutex) {
        g_bt_mesh_record_mutex = bt_mesh_os_layer_create_mutex();
        if (!g_bt_mesh_record_mutex) {
            LOG_E(mesh_os_layer, "create record mutex fail!!");
            return 0;
        }
    }
    return g_bt_mesh_record_mutex;
}
#endif /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */

bool bt_mesh_os_layer_read_record_data(const char *data_item_name, void *buffer, uint32_t *len)
{
    if (data_item_name == NULL || buffer == NULL || len == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_read_data, data_item_name = %p, buffer = %p, len = %p\n",
              data_item_name, buffer, len);
        return false;
    }

#ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM
    //1)Check cached record. If not found, add to nvdm and add to cache
    bool ret = false;
    uint32_t record_mutex = _bt_mesh_os_get_record_mutex();
    if (!record_mutex)
        return false;

    bt_mesh_os_layer_take_mutex(record_mutex);
    record_node_t *tmp_node = _bt_mesh_record_search_node_list(data_item_name);
    if (tmp_node) {
        //Return true if data in nvdm;otherwise, return false to like read invalid data from nvdm;
        if (tmp_node->item.data) {
            memcpy(buffer, tmp_node->item.data, tmp_node->item.data_len);
            *len = tmp_node->item.data_len;
            ret = true;
        }
        bt_mesh_os_layer_give_mutex(record_mutex);
        return ret;
    }
    //we need keep lock to prevent node status changed in between. Or do node search again.
    //bt_mesh_os_layer_give_mutex(record_mutex);

    nvdm_status_t status;
    status = nvdm_read_data_item("MESH", data_item_name, buffer, len);
    //2)Update data to cache no mater read pass or fail.
    //  This can prevent user read void nvdm again.
    if (status != NVDM_STATUS_OK) {
        _bt_mesh_record_add_new_node_to_tail(data_item_name, NULL, 0);
    } else {
        _bt_mesh_record_add_new_node_to_tail(data_item_name, (uint8_t *)buffer, *len);
        ret = true;
    }
    bt_mesh_os_layer_give_mutex(record_mutex);
    return ret;
#else /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */
    nvdm_status_t status;
    status = nvdm_read_data_item("MESH", data_item_name,  buffer, len);
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_read_record_data: status = %d", status);
        return false;
    }
    return true;
#endif /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */
}

bool bt_mesh_os_layer_write_record_data(const char *data_item_name, void *buffer, uint32_t len)
{
    nvdm_status_t status;

    if (data_item_name == NULL || buffer == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_write_data, data_item_name = %p, buffer = %p\n", data_item_name, buffer);
        return false;
    }

    status = MESH_OS_NVDM_WRITE_DATE_ITEM("MESH", data_item_name, NVDM_DATA_ITEM_TYPE_RAW_DATA, buffer, len);
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_write_record_data: status = %d", status);
        return false;
    }

#ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM
    //sync cached data.
    bool ret;
    uint32_t record_mutex = _bt_mesh_os_get_record_mutex();
    if (!record_mutex)
        return false;

    bt_mesh_os_layer_take_mutex(record_mutex);
    ret = _bt_mesh_record_update_node_by_id(data_item_name, (uint8_t *)buffer, len);
    bt_mesh_os_layer_give_mutex(record_mutex);
    return ret;
#else /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */
    return true;
#endif /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */
}

bool bt_mesh_os_layer_delete_all_record_data(void)
{
    nvdm_status_t status;

#ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM
    uint32_t record_mutex = _bt_mesh_os_get_record_mutex();
    if (!record_mutex)
        return false;
    bt_mesh_os_layer_take_mutex(record_mutex);
    _bt_mesh_record_free_node_list();
    bt_mesh_os_layer_give_mutex(record_mutex);
#endif /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */

    status = nvdm_delete_group("MESH");
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "%s: status = %d", __FUNCTION__, status);
        return false;
    }

    return true;
}

bool bt_mesh_os_layer_read_sequence_number_record_data(const char *data_item_name, void *buffer, uint32_t *len)
{
    nvdm_status_t status;

    if (data_item_name == NULL || buffer == NULL || len == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_read_number_data, data_item_name = %p, buffer = %p, len = %p\n",
              data_item_name, buffer, len);
        return false;
    }

#ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM
    bool ret = false;
    uint32_t record_mutex = _bt_mesh_os_get_record_mutex();
    if (!record_mutex)
        return false;

    bt_mesh_os_layer_take_mutex(record_mutex);
    if (g_seq_num_blk.data) {
        memcpy(buffer, g_seq_num_blk.data, g_seq_num_blk.data_len);
        *len = g_seq_num_blk.data_len;
        bt_mesh_os_layer_give_mutex(record_mutex);
        return true;
    }
    bt_mesh_os_layer_give_mutex(record_mutex);

    status = nvdm_read_data_item("MESH_SEQ", data_item_name,  buffer, len);

    bt_mesh_os_layer_take_mutex(record_mutex);
    if (NVDM_STATUS_OK == status) {
        if (!g_seq_num_blk.data)
            g_seq_num_blk.data = bt_mesh_os_layer_memory_alloc(*len);

        if (g_seq_num_blk.data) {
            memcpy(g_seq_num_blk.data, buffer, *len);
            g_seq_num_blk.data_len = *len;
        }
        ret = true;
    }
    bt_mesh_os_layer_give_mutex(record_mutex);
    return ret;
#else /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */
    status = nvdm_read_data_item("MESH_SEQ", data_item_name,  buffer, len);
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_read_sequence_number_record_data: status = %d", status);
        return false;
    }
    return true;
#endif /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */
}

bool bt_mesh_os_layer_write_sequence_number_record(const char *data_item_name, void *buffer, uint32_t len)
{
    nvdm_status_t status;

    if (data_item_name == NULL || buffer == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_write_number, data_item_name = %p, buffer = %p\n", data_item_name, buffer);
        return false;
    }

    status = MESH_OS_NVDM_WRITE_DATE_ITEM("MESH_SEQ", data_item_name, NVDM_DATA_ITEM_TYPE_RAW_DATA, buffer, len);
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_write_sequence_number_record: status = %d", status);
        return false;
    }

#ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM
    uint32_t record_mutex = _bt_mesh_os_get_record_mutex();
    if (!record_mutex)
        return false;

    bt_mesh_os_layer_take_mutex(record_mutex);
    if (!g_seq_num_blk.data)
        g_seq_num_blk.data = bt_mesh_os_layer_memory_alloc(len);
    if (g_seq_num_blk.data) {
        memcpy(g_seq_num_blk.data, buffer, len);
        g_seq_num_blk.data_len = len;
    }
    bt_mesh_os_layer_give_mutex(record_mutex);
#endif /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */

    return true;
}

bool bt_mesh_os_layer_delete_all_sequence_number_record(void)
{
#ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM
    uint32_t record_mutex = _bt_mesh_os_get_record_mutex();
    if (!record_mutex)
        return false;

    bt_mesh_os_layer_take_mutex(record_mutex);
    if (g_seq_num_blk.data)
        bt_mesh_os_layer_memory_free(g_seq_num_blk.data);
    memset(&g_seq_num_blk, 0x0, sizeof(seq_num_record_block_t));
    bt_mesh_os_layer_give_mutex(record_mutex);
#endif /* #ifdef BT_MESH_RECORD_DATA_CACAHE_IN_RAM */

    nvdm_status_t status;
    status = nvdm_delete_group("MESH_SEQ");
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_delete_sequence_number_record_data: status = %d", status);
        return false;
    }

    return true;
}

bool bt_mesh_os_layer_read_foundation_model_record_data(const char *data_item_name, void *buffer, uint32_t *len)
{
    nvdm_status_t status;

    if (data_item_name == NULL || buffer == NULL || len == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_read_foundation_data, data_item_name = %p, buffer = %p, len = %p\n",
              data_item_name, buffer, len);
    }

    status = nvdm_read_data_item("MESH_FOU", data_item_name,  buffer, len);
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_read_foundation_model_record_data: status = %d", status);
        return false;
    }

    return true;
}

bool bt_mesh_os_layer_write_foundation_model_record(const char *data_item_name, void *buffer, uint32_t len)
{
    nvdm_status_t status;

    if (data_item_name == NULL || buffer == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_write_foundation, data_item_name = %p, buffer = %p\n",
              data_item_name, buffer);
        return false;
    }

    status = MESH_OS_NVDM_WRITE_DATE_ITEM("MESH_FOU", data_item_name, NVDM_DATA_ITEM_TYPE_RAW_DATA, buffer, len);
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_write_foundation_model_record: status = %d", status);
        return false;
    }

    return true;
}

bool bt_mesh_os_layer_delete_all_foundation_model_record(void)
{
    nvdm_status_t status;
    status = nvdm_delete_group("MESH_FOU");
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_delete_all_foundation_model_record: status = %d", status);
        return false;
    }

    return true;
}

bool bt_mesh_os_layer_read_model_record_data(const char *data_item_name, void *buffer, uint32_t *len)
{
    nvdm_status_t status;

    if (data_item_name == NULL || buffer == NULL || len == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_read_model_data, data_item_name = %p, buffer = %p, len = %p\n",
              data_item_name, buffer, len);
        return false;
    }

    status = nvdm_read_data_item("MESH_MOD", data_item_name,  buffer, len);
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_read_model_record_data: status = %d", status);
        return false;
    }

    return true;
}

bool bt_mesh_os_layer_write_model_record(const char *data_item_name, void *buffer, uint32_t len)
{
    nvdm_status_t status;

    if (data_item_name == NULL || buffer == NULL) {
        LOG_E(mesh_os_layer, "mesh_os_write_model, data_item_name = %p, buffer = %p\n",
              data_item_name, buffer);
        return false;
    }

    status = MESH_OS_NVDM_WRITE_DATE_ITEM("MESH_MOD", data_item_name, NVDM_DATA_ITEM_TYPE_RAW_DATA, buffer, len);
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_write_model_record: status = %d", status);
        return false;
    }

    return true;
}

bool bt_mesh_os_layer_delete_all_model_record(void)
{
    nvdm_status_t status;
    status = nvdm_delete_group("MESH_MOD");
    if (NVDM_STATUS_OK != status) {
        //LOG_E(mesh_os_layer, "bt_mesh_os_layer_delete_all_model_record: status = %d", status);
        return false;
    }

    return true;
}

void bt_mesh_os_layer_ota_flash_apply_firmware(uint32_t data)
{
    return;
}

