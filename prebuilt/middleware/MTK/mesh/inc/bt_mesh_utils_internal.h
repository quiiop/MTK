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

#ifndef __BT_MESH_UTILS_INTERNAL_H__
#define __BT_MESH_UTILS_INTERNAL_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include "bt_mesh_utils.h"

/*!
    @brief A macro to return once the value is NULL.
*/
#define MESH_CHECK_NULL_RET(value)  \
    if (value == NULL) { \
        MESH_DEBUG_PRINTF(BT_MESH_DEBUG_UTILS, "%s is invalid\n", #value); \
        return; \
    }

/*!
    @brief A macro to return once the value is NULL.
*/
#define MESH_CHECK_NULL_RET_VALUE(value, ret)  \
    if (value == NULL) { \
        MESH_DEBUG_PRINTF(BT_MESH_DEBUG_UTILS, "%s is invalid\n", #value); \
        return ret; \
    }

/*!
    @brief A macro to free a non-null value and set it to NULL.
*/
#define MESH_CHECK_NULL_AND_FREE(value)  \
    if (value != NULL) { \
        bt_mesh_os_layer_memory_free(value); \
        value = NULL; \
    }

/*!
    @brief A macro to check null value and return BT_MESH_ERROR_OOM.
*/
#define MESH_CHECK_NULL_AND_RETURN_OOM(value)  \
    if (value == NULL) { \
        return BT_MESH_ERROR_OOM; \
    }

/*!
    @brief This function will copy source to destination in a reverse order
    @param[in] dst is the destination address
    @param[in] src is the source address
    @param[in] len is the length of data needed to be copied.
    @return NONE
*/
void mesh_utils_reverse_copy(uint8_t *dst, uint8_t *src, uint8_t len);

/*!
    @brief This function will revert byte order of target buffer
    @param[in] p is the source address of target buffer
    @param[in] len is the length of target buffer
    @return NONE
*/
void mesh_utils_byte_revert(uint8_t *p, uint16_t len);

/*!
    @brief This function do log field transformation according to Mesh Profile Specification v1.0 4.1.2
    @param[in] value is the 2-octet value needed to be transformed.
    @return the 1-octet log field value
*/
uint8_t mesh_utils_to_log_value(uint16_t value);

/*!
    @brief create the keylist array according to Mesh Profile Specification v1.0 4.3.1.1
    @param[in] outlen is a uint8_t pointer to pass the result array length
    @param[in] keyidxlist is a uint16_t array containing keys want to be combined
    @param[in] length is the length of @ref keyidxlist
    @return a uint8_t array, user should free this pointer
*/
uint8_t *mesh_utils_combine_key_index(uint32_t *outlen, uint16_t *keyidxlist, uint32_t length);

/*!
    @brief A tool to combine the application key index and the network key index into a 3-byte array.
    @param[out] out a pointer for a 3-byte array.
    @param[in] appkeyidx is the application key index.
    @param[in] netkeyidx is the network key index.
    @return NONE
*/
void mesh_utils_combine_app_net_key_index(uint8_t *out, uint16_t appkeyidx, uint16_t netkeyidx);

/*!
    @brief separate the keylist array according to Mesh Profile Specification v1.0 4.3.1.1
    @param[out] outlen a uint8_t pointer to pass the result array length
    @param[in] bytearray the byte array want to be separated into uint16_t key index list
    @param[in] length is the length of @ref bytearray
    @return a uint16_t key index array, user should free this pointer
*/
uint16_t *mesh_utils_separate_key_index(uint32_t *outlen, uint8_t *bytearray, uint32_t length);

/*!
    @brief a tool to separate application key index and network key index from a 3-byte array
    @param[in] in a pointer for 3-byte array
    @param[out] appkeyidx is a uint16_t pointer to pass the separated application key index
    @param[out] netkeyidx is a uint16_t pointer to pass the separated network key index
*/
void mesh_utils_separate_app_net_key_index(uint8_t *in, uint16_t *appkeyidx, uint16_t *netkeyidx);

/*!
    @brief a tool to extract fields from a payload buffer.
    @param[in] in A pointer for payload array.
    @param[in] format Format indicator. It's a null-terminated string. Every field was prepresented by %x, where x was predefined type
    characters. Here list all the types characters:
    -  b : uint8_t.
    -  w : uint16_t.
    -  d : uint32_t.
    -  W : uint16_t from big endian.
    -  D : uint32_t from big endian.
    Number could be added in front of type characters, like 3b or 5w, which means three entries of uint8_t array and five entries of uint16_t array.

    The variadic arguments need to match the type indicated by format argument. Here is an example:
    @code
        in[] = {0x22, 0x11, 0x33, 0x77, 0x66, 0x55, 0x44, 0x22, 0x11, 0x33, 0x44, 0x55, 0x66, 0x77,
                0x24, 0x25, 0x22, 0x23, 0x11, 0x12, 0x13, 0x41, 0x42, 0x43, 0x44, 0x52, 0x53, 0x54,
                0x55, 0x25, 0x24, 0x23, 0x22, 0x11, 0x12, 0x13, 0x44, 0x43, 0x42, 0x41, 0x55, 0x54, 0x53, 0x52};

        uint8_t u8[8];
        uint16_t u16[6];
        uint32_t u32[6];

        mesh_utils_separate_fields(in, "wbd WBD 2w3b2d 2W3B2D", &u16[0], &u8[0], &u32[0], &u16[1], &u8[1], &u32[1], &u16[2], &u8[2], &u32[2], &u16[4], &u8[5], &u32[4]);

       //u8[] should be 0x33, 0x33, 0x11, 0x12, 0x13, 0x11, 0x12, 0x13
       //u16[] should be 0x1122, 0x2211, 0x2524 0x2322, 0x4241, 0x4443 0x5352, 0x2524 0x2322
       //u32[] should be 0x44556677, 0x44556677, 0x44434241 0x55545352, 0x44434241, 0x55545352

    @endcode
*/
uint8_t *mesh_utils_separate_fields(uint8_t *in, char *format, ...);

/*!
    @brief a tool to fill payload buffer.
    @param[in] in A pointer for payload array.
    @param[in] format Format indicator. It's a null-terminated string. Every field was prepresented by x, where x was predefined defined
    characters. Here list all the types characters:
    -  b : uint8_t.
    -  w : uint16_t.
    -  d : uint32_t.
    -  W : uint16_t from big endian.
    -  D : uint32_t from big endian.
    Number could be added in front of type characters, like 3b or 5w, which means three entries of uint8_t array and five entries of uint16_t array.

    The variadic arguments need to match the type indicated by format argument. Here is an example:
    @code
        uint8_t in[100];
        uint8_t u8 = 0x33;
        uint16_t u16 = 0x1122;
        uint32_t u32 = 0x44556677;
        uint8_t p_u8[3] = {0x11, 0x12, 0x13};
        uint16_t p_u16[2] = {0x2524, 0x2322};
        uint32_t p_u32[2] = {0x44434241, 0x55545352};

        mesh_utils_assemble_fields(in, "wbdWBD2w3b2d2W3B2D", u16, u8, u32, u16, u8, u32);

        //in[] would be
        //0x22, 0x11, 0x33, 0x77, 0x66, 0x55, 0x44, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77
        //0x24, 0x25, 0x22, 0x23, 0x11, 0x12, 0x13, 0x41, 0x42, 0x43, 0x44, 0x52, 0x53, 0x54,
        //0x55, 0x25, 0x24, 0x23, 0x22, 0x11, 0x12, 0x13, 0x44, 0x43, 0x42, 0x41, 0x55, 0x54, 0x53, 0x52
    @endcode


*/
uint8_t *mesh_utils_assemble_fields(uint8_t *in, char *format, ...);


/*
    @brief  mesh_utils_assemble_fields with va_list version.
*/
uint8_t *mesh_utils_assemble_fields_v(uint8_t *in, char *format, va_list ap);

/*!
@}
*/

#endif // __BT_MESH_UTILS_INTERNAL_H__

