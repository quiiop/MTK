/* Copyright Statement:
 *
 * (C) 2005-2030  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ci_cli.h"
#include "ci.h"
#include "hal.h"
#include "hal_aes.h"
#include "hal_des.h"
#include "hal_sha.h"
#include "hal_md5.h"

#define CIPH_BUFFER_SIZE        (1024)
#define CIPH_AES_KEY_SIZE   (16)
#define CIPH_DES_KEY_SIZE   (24)

static const uint8_t ciph_data_aes_key[] = {
    0x4d, 0x54, 0x4b, 0x30, 0x30, 0x30, 0x30, 0x30, 0x32, 0x30, 0x31, 0x34, 0x30, 0x38, 0x31, 0x35,
};

static uint8_t aes_iv[] = {
    0x61, 0x33, 0x46, 0x68, 0x55, 0x38, 0x31, 0x43, 0x77, 0x68, 0x36, 0x33, 0x50, 0x76, 0x33, 0x46,
};

static const uint8_t ciph_data_aes_in[] = {
    0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37, 0x42, 0x4d, 0x58, 0x63, 0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37,
    0x42, 0x4d, 0x58, 0x63, 0x00, 0x0b, 0x16, 0x21, 0x2c, 0x37, 0x42, 0x4d, 0x58, 0x63, 0x00, 0x00,
};

static const uint8_t ciph_data_aes_ecb_out[] = {
    0x3f, 0x70, 0xf3, 0xca, 0x70, 0x92, 0x91, 0xb1, 0xfc, 0xf7, 0x5d, 0x2a, 0xa8, 0x0f, 0xfe, 0x47,
    0x77, 0x33, 0xfa, 0x01, 0xa6, 0x8d, 0x0a, 0x55, 0xfc, 0xaa, 0x29, 0x3a, 0x1b, 0x9a, 0x5e, 0x78,
};

static const uint8_t ciph_data_aes_ctr_out[] = {
    0xA2, 0x3A, 0x07, 0x69, 0xBE, 0xDE, 0xF8, 0xD2, 0xEB, 0xAC, 0x29, 0xB0, 0x17, 0x85, 0x6C, 0x7C,
    0x03, 0x08, 0x11, 0x29, 0x09, 0x18, 0x5a, 0x13, 0xBE, 0x5B, 0x63, 0xAE, 0x78, 0x05, 0x4F, 0x3D,
};

static const uint8_t ciph_data_aes_cbc_out[] = {
    0x85, 0x50, 0xcd, 0x40, 0xe9, 0xe5, 0xc9, 0xf0, 0xe6, 0xa3, 0x3f, 0x7a, 0x76, 0x68, 0xaa, 0x11,
    0x6e, 0xcd, 0x8c, 0x12, 0xf6, 0x15, 0x26, 0xec, 0x30, 0x43, 0x12, 0xab, 0x16, 0xa7, 0x40, 0x66,
};

/* aes-gcm only */
static const uint8_t ciph_data_aes_in_gcm[60] = { 0 };
static const uint8_t ciph_gcm_key[16] = { 0 };
static uint8_t gcm_iv[16] = { 0 };
static uint8_t gcm_aad[16] = { 0 };
static uint8_t gcm_tag[16] = { 0 };

static const uint8_t ciph_data_aes_gcm_out[] = {
    0xa3, 0xb2, 0x2b, 0x84, 0x49, 0xaf, 0xaf, 0xbc, 0xd6, 0xc0, 0x9f, 0x2c, 0xfa, 0x9d, 0xe2, 0xbe,
    0x93, 0x8f, 0x8b, 0xbf, 0x23, 0x58, 0x63, 0xd0, 0xce, 0x02, 0x84, 0x27, 0x22, 0xfd, 0x50, 0x34,
    0x89, 0x0a, 0xd0, 0xf9, 0xf1, 0x4d, 0xdd, 0x88, 0xd0, 0x6e, 0x87, 0x1e, 0x48, 0x77, 0x49, 0xfa,
    0x7f, 0x7f, 0xf9, 0x35, 0x5c, 0x0a, 0x18, 0xad, 0xba, 0x85, 0x87, 0x44,
};

static const uint8_t ciph_gcm_tag_golden[] = {
    0x1d, 0xc9, 0xe0, 0x4d, 0xe8, 0x87, 0x5d, 0x2e, 0x81, 0x14, 0x1f, 0xc3, 0x69, 0x0b, 0x47, 0x32,
};

/* des only */
static const uint8_t des_ecb_key[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
};

static const uint8_t ciph_data_des_in1[] = {
    0x82, 0x73, 0xE2, 0x04, 0xF1, 0x66, 0xFA, 0xE2, 0xEC, 0x50, 0xB4, 0xD8, 0x39, 0xAC, 0x03, 0x73,
    0xDC, 0x03, 0x44, 0xB8, 0x09, 0x3E, 0x4D, 0x12, 0xF2, 0x87, 0xB6, 0xE5, 0x9C, 0x81, 0x12, 0x39,
    0x15, 0x89, 0x0F, 0x2B, 0xD0, 0xC5, 0xDC, 0xA1, 0x64, 0x1B, 0x85, 0xC8, 0x40, 0x90, 0x5D, 0x81,
    0x50, 0xD5, 0xF9, 0xA1, 0x13, 0xB5, 0x55, 0x27, 0x82, 0x22, 0x3A, 0x04, 0x8F, 0x8C, 0x6E, 0x1F,
    0xCB, 0x5C, 0x80, 0x96, 0x2C, 0x2D, 0x23, 0x58, 0x89, 0x19, 0xB4, 0x12, 0x0A, 0x6E, 0x93, 0x14,
    0xD8, 0xAC, 0xE2, 0xB1, 0xCB, 0x22, 0x70, 0x96, 0xF6, 0xC3, 0x8E, 0xED, 0x2A, 0xC2, 0xD5, 0x55,
    0x7A, 0xB1, 0x1B, 0xF5, 0x15, 0x8B, 0x2B, 0x2B, 0x93, 0xC1, 0x22, 0x27, 0x56, 0xB4, 0x03, 0xAD,
    0xD3, 0xA8, 0xE4, 0xA7, 0x42, 0x49, 0xFF, 0x84, 0xDC, 0xEF, 0x90, 0xB9, 0xF3, 0xD7, 0xA6, 0xE7,
    0x6D, 0xE5, 0xBB, 0xDB, 0xA4, 0xDA, 0x56, 0x49, 0x28, 0x83, 0xB1, 0x51, 0x64, 0xBA, 0x0F, 0xC9,
    0x15, 0x46, 0xDD, 0x2A, 0xD0, 0xBC, 0x5E, 0xA1, 0xA2, 0xE6, 0x20, 0x45, 0x7D, 0x2A, 0x45, 0xFA,
    0xBE, 0x75, 0x42, 0x7C, 0x41, 0xA1, 0x05, 0x83, 0xFF, 0x4F, 0x3C, 0xFE, 0xB9, 0x46, 0x14, 0x72,
    0xB7, 0xDF, 0xAA, 0x6F, 0xB4, 0x55, 0xF2, 0x68, 0x78, 0x23, 0x21, 0xF0, 0x43, 0x46, 0x0D, 0x86,
    0x20, 0x76, 0x91, 0x40, 0x1C, 0x71, 0x97, 0x38, 0x7B, 0x0D, 0xA9, 0xF6, 0x94, 0x09, 0x76, 0x28,
    0x60, 0x28, 0x31, 0xC0, 0xE2, 0xBF, 0x1A, 0xC5, 0x1C, 0xDE, 0x71, 0x39, 0x3A, 0x62, 0x5F, 0x74,
    0x1E, 0x1C, 0x87, 0x3C, 0xD8, 0x6A, 0x6A, 0xB0, 0x31, 0x24, 0xD3, 0x62, 0x15, 0x21, 0x93, 0x2A,
    0xF0, 0xB1, 0x4E, 0xE1, 0x0A, 0xEC, 0x35, 0x15, 0xA1, 0x8B, 0xEE, 0x43, 0x0B, 0xE2, 0x9D, 0x17,
    0xC3, 0x36, 0x04, 0x86, 0x64, 0xB5, 0xE3, 0xC9, 0x60, 0xF6, 0x9D, 0xC1, 0x12, 0x97, 0xCB, 0x25,
    0xAD, 0x67, 0xE2, 0x5A, 0xC6, 0x9D, 0xA2, 0x8D, 0x16, 0x5B, 0x7B, 0x2C, 0x03, 0xD6, 0x27, 0x07,
    0xB8, 0xAD, 0xE6, 0x71, 0x06, 0x0A, 0x5D, 0x0C,
};

static const uint8_t ciph_data_des_ecb_out[] = {
    0xCA, 0xCD, 0xCF, 0x74, 0xA2, 0xC6, 0x73, 0xDE, 0xE2, 0xFE, 0xC4, 0xE6, 0x8F, 0x4E, 0x5C, 0xEE,
    0x1D, 0x65, 0x67, 0x5B, 0xD3, 0x3F, 0x32, 0x6E, 0xBF, 0x6B, 0xEE, 0x10, 0xAA, 0x7B, 0xCD, 0xFE,
    0xD3, 0x03, 0x61, 0x66, 0x18, 0x2D, 0x8B, 0x05, 0x77, 0x39, 0x9A, 0xE8, 0x40, 0x76, 0xC7, 0xF5,
    0xDC, 0xD0, 0x13, 0xFF, 0x25, 0x4B, 0x3D, 0x38, 0x3B, 0xD0, 0x72, 0xA0, 0x46, 0x21, 0x95, 0xF8,
    0x19, 0x4E, 0x45, 0x3D, 0x5C, 0x58, 0xB0, 0x63, 0xE6, 0x10, 0x12, 0x33, 0x43, 0xBB, 0xC8, 0x51,
    0xA9, 0x42, 0x50, 0x1A, 0x52, 0xA7, 0x95, 0xE2, 0xC7, 0x24, 0xF0, 0x2E, 0xC3, 0xD4, 0x53, 0xC7,
    0xA8, 0x7E, 0x72, 0xDA, 0x61, 0x38, 0xDE, 0x12, 0x37, 0x0F, 0xD0, 0x86, 0x93, 0xAD, 0x7F, 0x96,
    0x9E, 0x42, 0x97, 0xA1, 0xF4, 0x06, 0x44, 0x11, 0x9E, 0xAA, 0x9D, 0xC6, 0xE3, 0x24, 0xBC, 0x7C,
    0x09, 0x93, 0x48, 0x80, 0x34, 0x80, 0x2F, 0x01, 0x50, 0xB0, 0xDD, 0xF5, 0xE3, 0xA3, 0xA2, 0x9F,
    0x25, 0x60, 0x55, 0x8B, 0x8F, 0x1E, 0x01, 0xCF, 0x89, 0x10, 0x04, 0xD5, 0x34, 0xB2, 0x47, 0x36,
    0xA3, 0x0A, 0x72, 0xDF, 0x87, 0x47, 0x38, 0x77, 0x6E, 0x5A, 0x4D, 0x87, 0x24, 0x0F, 0xB7, 0x55,
    0x56, 0xAF, 0x78, 0xC3, 0x7B, 0x26, 0x94, 0xE2, 0x52, 0x17, 0x17, 0x13, 0xF1, 0x32, 0xDB, 0xF5,
    0xE2, 0x87, 0x8D, 0xB7, 0xDA, 0x29, 0x66, 0x2B, 0x0A, 0x93, 0x0D, 0x54, 0xEB, 0x98, 0x8F, 0xCB,
    0xE3, 0x2D, 0x15, 0x55, 0xC9, 0xC0, 0x00, 0x3C, 0xB6, 0xBF, 0x2C, 0x16, 0x3E, 0x82, 0xC5, 0xC3,
    0x37, 0x6D, 0x2A, 0x2D, 0xD4, 0x8C, 0x3A, 0x75, 0xCA, 0xAC, 0xE0, 0x46, 0x84, 0xBA, 0x4F, 0x51,
    0xCD, 0x48, 0xB3, 0x28, 0x9A, 0x26, 0xBB, 0x0B, 0x4D, 0xCF, 0x41, 0x98, 0xE8, 0x5C, 0x89, 0x9F,
    0x70, 0x6D, 0x58, 0xEE, 0xA1, 0x77, 0x2A, 0x46, 0x53, 0x99, 0x6E, 0x3F, 0xC3, 0x13, 0x96, 0x74,
    0x34, 0x4F, 0xB2, 0x47, 0x95, 0x03, 0x85, 0x3A, 0x1B, 0x65, 0x3F, 0x8E, 0x63, 0xCD, 0x56, 0xC7,
    0x53, 0xFD, 0xB1, 0xF7, 0x37, 0xE0, 0x92, 0xAC,
};

static const uint8_t des_cbc_key[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xf1, 0xe0, 0xd3, 0xc2, 0xb5, 0xa4, 0x97, 0x86,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
};

static uint8_t des_iv[] = {
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
};

static const uint8_t ciph_data_des_in2[] = {
    0x1D, 0x06, 0x33, 0x3A, 0x9D, 0xCC, 0x60, 0x3B, 0x0A, 0xAA, 0x4B, 0x15, 0xBF, 0x23, 0x32, 0x7E,
    0x6C, 0x9C, 0x4B, 0xD9, 0x78, 0xD9, 0x9B, 0xF1, 0x26, 0x35, 0xFB, 0x4C, 0x31, 0x23, 0x07, 0x62,
    0x47, 0xB9, 0xA1, 0x8F, 0x4F, 0xA1, 0x55, 0x9F, 0xDB, 0xCD, 0x60, 0xB7, 0x7C, 0x6D, 0xA9, 0xF8,
    0xAE, 0x7D, 0x94, 0x5C, 0xCF, 0xC4, 0x56, 0x9F, 0x44, 0xDE, 0x37, 0x89, 0xAD, 0xCB, 0xC0, 0x5B,
    0xE8, 0x23, 0x3E, 0xD0, 0xA4, 0xAE, 0x32, 0x49, 0xA8, 0xC7, 0xFC, 0x51, 0xEB, 0xFE, 0xC0, 0xD7,
    0x06, 0x1D, 0x7F, 0x0C, 0x4B, 0x21, 0x2F, 0x96, 0x64, 0x57, 0xED, 0xC8, 0xE1, 0x2C, 0xCE, 0xC2,
    0x1E, 0xF2, 0xED, 0x3D, 0xBB, 0x82, 0x5A, 0x77, 0xBF, 0x05, 0x00, 0x7E, 0x09, 0x0C, 0xDB, 0x12,
    0x1C, 0xD9, 0xE7, 0x39, 0x36, 0xE5, 0x81, 0x6D, 0xE9, 0xEB, 0xF6, 0xD3, 0x0F, 0xD2, 0x8F, 0x1F,
    0xBC, 0x14, 0x88, 0x78, 0x42, 0xD9, 0x2F, 0x85, 0x24, 0x8B, 0x4B, 0x49, 0x2D, 0xDA, 0x59, 0x5B,
    0x15, 0x0C, 0xAF, 0x2A, 0x9F, 0xF2, 0xAE, 0x3F, 0xC3, 0x3F, 0x38, 0x86, 0x2C, 0x15, 0x63, 0x58,
    0xCE, 0x2F, 0xF2, 0x9C, 0xC7, 0x1A, 0x0C, 0x8E, 0x93, 0x7D, 0xBC, 0x26, 0x7D, 0x35, 0x99, 0xFA,
    0xD1, 0x8B, 0xB4, 0xA3, 0x39, 0x96, 0x17, 0x73, 0xD9, 0xD2, 0x92, 0xB3, 0x97, 0x99, 0xA8, 0x2F,
    0x22, 0x29, 0x0D, 0x44, 0xAF, 0xD8, 0x56, 0x5F, 0xE5, 0x9B, 0x36, 0xCB, 0x7B, 0xF8, 0xFD, 0xF7,
    0x2D, 0x28, 0xDB, 0x5A, 0xF3, 0x09, 0x1A, 0xE6, 0xF6, 0x82, 0xE2, 0xED, 0x14, 0xCF, 0xC1, 0x29,
    0xB2, 0x9F, 0xB6, 0x65, 0xED, 0x53, 0x6C, 0xDA, 0xEF, 0xBE, 0x94, 0xDF, 0xDA, 0x8A, 0xE2, 0xB5,
    0x22, 0x31, 0xFF, 0x44, 0x3C, 0xF1, 0x19, 0x79, 0x15, 0x09, 0x0B, 0x2A, 0xE8, 0x74, 0x0C, 0xD0,
    0x07, 0x6F, 0xCF, 0x0E, 0x55, 0xF6, 0xAD, 0xAB,
};

static const uint8_t ciph_data_des_cbc_out[] = {
    0xAE, 0x4D, 0x98, 0x64, 0xDE, 0x5C, 0xD9, 0x1E, 0x65, 0x3D, 0xD8, 0x7A, 0xE1, 0x0A, 0x36, 0x80,
    0xD4, 0x55, 0xFD, 0xFA, 0x6C, 0x0C, 0x86, 0xF2, 0x4D, 0xCC, 0x9A, 0x66, 0x62, 0x9D, 0x2C, 0x65,
    0x6E, 0x75, 0x82, 0xD0, 0x42, 0x2F, 0x11, 0xE4, 0x90, 0xEE, 0x94, 0x9B, 0x9C, 0xF7, 0xAA, 0x9E,
    0x0A, 0xB5, 0x1A, 0xBE, 0x51, 0xCA, 0x0D, 0xA8, 0xC6, 0xA4, 0xF4, 0x05, 0x54, 0xB4, 0x4A, 0x70,
    0xDE, 0x64, 0xE6, 0x13, 0xF5, 0x87, 0xB3, 0xFF, 0x59, 0xE4, 0xEF, 0x7E, 0xC8, 0xB7, 0x91, 0x81,
    0x47, 0x88, 0x41, 0xC7, 0x6A, 0x75, 0xE8, 0xEF, 0xF4, 0xEC, 0xAE, 0xDD, 0xBC, 0x24, 0xCF, 0x37,
    0x71, 0x9B, 0x38, 0xE6, 0x7D, 0x5A, 0xA7, 0xE7, 0xAC, 0x3C, 0x91, 0x60, 0x4E, 0x56, 0xEB, 0x58,
    0x77, 0xC8, 0x3A, 0x09, 0xAB, 0x74, 0x0B, 0x73, 0x9A, 0xD4, 0xC3, 0xC2, 0xF1, 0xD4, 0xE8, 0x45,
    0x0E, 0x3B, 0x20, 0x71, 0x17, 0x8A, 0xA5, 0x28, 0xFA, 0x8A, 0x0C, 0x69, 0xFF, 0x10, 0x53, 0xB4,
    0xF3, 0x18, 0x50, 0xD6, 0x28, 0xF2, 0x83, 0x52, 0x6D, 0x34, 0xAF, 0xE6, 0xE2, 0x0C, 0x9E, 0xB0,
    0x4B, 0x49, 0xB0, 0x46, 0x96, 0xF3, 0xD8, 0xB4, 0x54, 0xEE, 0xE4, 0xC7, 0xA0, 0xB7, 0x0E, 0x84,
    0xBB, 0x83, 0x09, 0x4B, 0x6C, 0x72, 0xA8, 0xD1, 0xAB, 0x7C, 0x27, 0x85, 0x78, 0xC0, 0x1C, 0x44,
    0x62, 0x33, 0x93, 0x0A, 0xE7, 0xFE, 0x21, 0x55, 0xD4, 0x68, 0x37, 0x8D, 0x0F, 0x4D, 0x04, 0xF0,
    0x84, 0x9B, 0xBA, 0xE5, 0xD1, 0x77, 0x7A, 0xF0, 0xAC, 0x65, 0x05, 0xED, 0x12, 0x0E, 0xD8, 0x1D,
    0x24, 0xCD, 0xE9, 0xDA, 0x7D, 0x4D, 0xFC, 0x4A, 0x68, 0x82, 0x6D, 0x04, 0xDF, 0x61, 0x36, 0x46,
    0xA5, 0xAA, 0x44, 0xDA, 0x02, 0x2C, 0x45, 0x3E, 0xC2, 0x6E, 0xAC, 0xDC, 0x49, 0x2D, 0x1D, 0xF9,
    0x84, 0x53, 0xA0, 0x2B, 0x72, 0x71, 0xC6, 0x9A, 0xfe, 0x34, 0xfa, 0x35, 0x8a, 0x2d, 0x4e, 0x41,
    0x0e, 0xea, 0x6c, 0x29, 0xb2, 0x88, 0x08,
};

/* sha only */
static const uint8_t hash_data_sha1_in[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
};

static const uint8_t hash_data_sha1_out[] = {
    0x4b, 0x98, 0x92, 0xb6, 0x52, 0x72, 0x14, 0xaf,
    0xc6, 0x55, 0xb8, 0xaa, 0x52, 0xf4, 0xd2, 0x03,
    0xc1, 0x5e, 0x7c, 0x9c,
};

static const uint8_t hash_data_sha224_in[] = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
};

static const uint8_t hash_data_sha224_out[] = {
    0x08, 0x21, 0x69, 0xf9, 0x77, 0x1b, 0x80, 0x15,
    0xf3, 0x97, 0xae, 0xde, 0x5b, 0xba, 0xa2, 0x72,
    0x2d, 0x8f, 0x5c, 0x19, 0xfe, 0xd2, 0xe2, 0x68,
    0x92, 0x49, 0xd8, 0x44,
};

static const uint8_t hash_data_sha256_in[] = { 'a', 'b', 'c' };

static const uint8_t hash_data_sha256_out[] = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
    0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
    0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
    0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
};

/* md5 only */
static uint8_t hash_data_md5_in[] = {
    'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
};

static const uint8_t hash_data_md5_out[] = {
    0x61, 0x12, 0x71, 0x83, 0x70, 0x8d, 0x3a, 0xc7,
    0xf1, 0x9b, 0x66, 0x06, 0xfc, 0xae, 0x7d, 0xf6,
};

ci_status_t ci_gcpu_crypto_aes_sample(void)
{
    UINT8 *pbuf, *cbuf, *golden, *key;
    hal_aes_buffer_t plain_text, cipher_text, key_text;
    UINT32 i = 0, ci_ret = CI_FAIL;

    key = (UINT8 *)malloc(CIPH_AES_KEY_SIZE);
    if (key == NULL) {
        printf("key buffer alloc failed\n");
        return CI_FAIL;
    }

    golden = (UINT8 *)malloc(CIPH_BUFFER_SIZE);
    if (golden == NULL) {
        printf("output buffer alloc failed\n");
        goto FAIL_KEY;
    }

    pbuf = (UINT8 *)malloc(CIPH_BUFFER_SIZE);
    if (pbuf == NULL) {
        printf("plain_text buffer alloc failed\n");
        goto FAIL_GOLDEN;
    }

    cbuf = (UINT8 *)malloc(CIPH_BUFFER_SIZE);
    if (cbuf == NULL) {
        printf("cipher_text buffer alloc failed\n");
        goto FAIL_PBUF;
    }

    /* AES ECB/CBC/CTR Test key */
    memcpy(key, ciph_data_aes_key, sizeof(ciph_data_aes_key));
    key_text.buffer = key;
    key_text.length = CIPH_AES_KEY_SIZE;

    /* ECB mode.
     * Notice in ecb mode, same plain text will output same cipher
     * text after encryption.
     */
    plain_text.buffer = pbuf;
    plain_text.length = CIPH_BUFFER_SIZE;
    cipher_text.buffer = cbuf;
    cipher_text.length = CIPH_BUFFER_SIZE;

    /* ecb encryption */
    /* in:  plain_text, content: ciph_data_aes_in*n
     * out: cipher_text
     * golden: ciph_data_aes_ecb_out*n
     */
    for (i = 0; i < (CIPH_BUFFER_SIZE / sizeof(ciph_data_aes_in)); i++) {
        memcpy((plain_text.buffer + 32 * i), ciph_data_aes_in, sizeof(ciph_data_aes_in));
    }
    if (hal_aes_ecb_encrypt(&cipher_text, &plain_text, &key_text) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }

    /* compare the result. */
    for (i = 0; i < (CIPH_BUFFER_SIZE / sizeof(ciph_data_aes_ecb_out)); i++) {
        memcpy((golden + 32 * i), ciph_data_aes_ecb_out, sizeof(ciph_data_aes_ecb_out));
    }
    if (!memcmp(cipher_text.buffer, golden, cipher_text.length)) {
        printf("AES ECB encryption test pass\n");
    } else {
        printf("AES ECB encryption test fail\n");
        // debug
        for (uint32_t i = 0; i < cipher_text.length; i++) {
            printf("result: 0x%x, golden: 0x%x\n", cipher_text.buffer[i], golden[i]);
        }
        // debug end
        goto FAIL_OUT;
    }

    /* ecb decryption */
    /* in:  cipher_text, content: ciph_data_aes_ecb_out*n
     * out: plain_text, content: ciph_data_aes_in*n
     * golden: ciph_data_aes_in*n
     */
    if (hal_aes_ecb_decrypt(&plain_text, &cipher_text, &key_text) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }

    /* compare the result. */
    for (i = 0; i < (CIPH_BUFFER_SIZE / sizeof(ciph_data_aes_in)); i++) {
        memcpy((golden + 32 * i), ciph_data_aes_in, sizeof(ciph_data_aes_in));
    }
    if (!memcmp(plain_text.buffer, golden, plain_text.length)) {
        printf("AES ECB decryption test pass\n");
    } else {
        printf("AES ECB decryption test fail\n");
        goto FAIL_OUT;
    }

    /* ecb encryption ex */
    /* in:  plain_text, content: ciph_data_aes_in*n
     * out: cipher_text, content: ciph_data_aes_ecb_out*n
     */
    if (hal_aes_ecb_encrypt_ex(&cipher_text, &plain_text, &key_text, 0) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    /* ecb decryption ex */
    /* in:  cipher_text, content: ciph_data_aes_ecb_out*n
     * out: plain_text, content: ciph_data_aes_in*n
     * golden: ciph_data_aes_in*n
     */
    if (hal_aes_ecb_decrypt_ex(&plain_text, &cipher_text, &key_text, 0) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(plain_text.buffer, golden, plain_text.length)) {
        printf("AES ECB encrpt/decrypt ex test pass\n");
    } else {
        printf("AES ECB encrpt/decrypt ex test fail\n");
        goto FAIL_OUT;
    }

    /* CTR mode */
    plain_text.buffer = pbuf;
    plain_text.length = sizeof(ciph_data_aes_in);
    cipher_text.buffer = cbuf;
    cipher_text.length = sizeof(ciph_data_aes_ctr_out);

    /* ctr encryption */
    /* in:  plain_text, content: ciph_data_aes_in
     * out: cipher_text, content: ciph_data_aes_ctr_out
     */
    memcpy(plain_text.buffer, ciph_data_aes_in, sizeof(ciph_data_aes_in));

    if (hal_aes_ctr(&cipher_text, &plain_text, &key_text, aes_iv) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(cipher_text.buffer, ciph_data_aes_ctr_out, cipher_text.length)) {
        printf("AES CTR encrypt/decrypt test pass\n");
    } else {
        printf("AES CTR encrypt/decrypt test fail\n");
        goto FAIL_OUT;
    }

    /* ctr decryption */
    /* in:  cipher_text, content: ciph_data_aes_ctr_out
     * out: plain_text, content: ciph_data_aes_in
     */
    memset(plain_text.buffer, 0, plain_text.length);

    if (hal_aes_ctr(&plain_text, &cipher_text, &key_text, aes_iv) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(plain_text.buffer, ciph_data_aes_in, plain_text.length)) {
        printf("AES CTR encrypt/decrypt test pass\n");
    } else {
        printf("AES CTR encrypt/decrypt test fail\n");
        goto FAIL_OUT;
    }

    /* CBC mode */
    /* cbc encryption */
    /* in:  plain_text, content: ciph_data_aes_in
     * out: cipher_text, content: ciph_data_aes_cbc_out
     */
    memcpy(plain_text.buffer, ciph_data_aes_in, sizeof(ciph_data_aes_in));

    if (hal_aes_cbc_encrypt(&cipher_text, &plain_text, &key_text, aes_iv) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(cipher_text.buffer, ciph_data_aes_cbc_out, cipher_text.length)) {
        printf("AES CBC encrypt test pass\n");
    } else {
        printf("AES CBC encrypt test fail\n");
        goto FAIL_OUT;
    }

    /* cbc encryption */
    /* in:  cipher_text, content: ciph_data_aes_cbc_out
     * out: plain_text, content: ciph_data_aes_in
     */
    memset(plain_text.buffer, 0, plain_text.length);

    if (hal_aes_cbc_decrypt(&plain_text, &cipher_text, &key_text, aes_iv) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(plain_text.buffer, ciph_data_aes_in, plain_text.length)) {
        printf("AES CBC decrypt test pass\n");
    } else {
        printf("AES CBC decrypt test fail\n");
        goto FAIL_OUT;
    }

    /* cbc encryption/decryption ex */
    memcpy(plain_text.buffer, ciph_data_aes_in, sizeof(ciph_data_aes_in));

    if (hal_aes_cbc_encrypt_ex(&cipher_text, &plain_text, &key_text, aes_iv, 0) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (hal_aes_cbc_decrypt_ex(&plain_text, &cipher_text, &key_text, aes_iv, 0) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(plain_text.buffer, ciph_data_aes_in, plain_text.length)) {
        printf("AES CBC encrypt/decrypt ex test pass\n");
    } else {
        printf("AES CBC encrypt/decrypt ex test fail\n");
        goto FAIL_OUT;
    }

    /* cbc iteration mode */
    hal_aes_buffer_t cbc_plain_text1, cbc_plain_text2;
    hal_aes_buffer_t cbc_cipher_text1, cbc_cipher_text2;
    UINT8 *iv_tmp;

    cbc_plain_text1.buffer = pbuf;
    cbc_plain_text1.length = sizeof(ciph_data_aes_in) / 2;
    cbc_plain_text2.buffer = pbuf + (sizeof(ciph_data_aes_in) / 2);
    cbc_plain_text2.length = sizeof(ciph_data_aes_in) / 2;
    cbc_cipher_text1.buffer = cbuf;
    cbc_cipher_text1.length = sizeof(ciph_data_aes_cbc_out) / 2;
    cbc_cipher_text2.buffer = cbuf + (sizeof(ciph_data_aes_cbc_out) / 2);
    cbc_cipher_text2.length = sizeof(ciph_data_aes_cbc_out) / 2;

    /* cbc iteration encryption */
    /* in:  plain_text, content: ciph_data_aes_in
     * out: cipher_text, content: ciph_data_aes_cbc_out
     */
    memset(plain_text.buffer, 0, plain_text.length);
    memcpy(pbuf, ciph_data_aes_in, sizeof(ciph_data_aes_in));
    /* IV tmp */
    iv_tmp = pbuf + sizeof(ciph_data_aes_in);
    memcpy(iv_tmp, aes_iv, sizeof(aes_iv));
    if (hal_aes_cbc_encrypt_iteration(&cbc_cipher_text1, &cbc_plain_text1, &key_text, iv_tmp) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    /* iv_tmp is changed: is the value of previous XOR */
    if (hal_aes_cbc_encrypt_iteration(&cbc_cipher_text2, &cbc_plain_text2, &key_text, iv_tmp) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }

    if (!memcmp(cbc_cipher_text1.buffer, ciph_data_aes_cbc_out, cbc_cipher_text1.length) &&
        !memcmp(cbc_cipher_text2.buffer, (ciph_data_aes_cbc_out + cbc_cipher_text1.length), cbc_cipher_text2.length)) {
        printf("AES CBC iteration encrypt test pass\n");
    } else {
        printf("AES CBC iteration encrypt test fail\n");
        goto FAIL_OUT;
    }

    /* cbc iteration decryption */
    /* in:  cipher_text, content: ciph_data_aes_cbc_out
     * out: plain_text, content: ciph_data_aes_in
     */
    memset(pbuf, 0, sizeof(ciph_data_aes_in));
    /* restore IV tmp */
    memcpy(iv_tmp, aes_iv, sizeof(aes_iv));
    if (hal_aes_cbc_decrypt_iteration(&cbc_plain_text1, &cbc_cipher_text1, &key_text, iv_tmp) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (hal_aes_cbc_decrypt_iteration(&cbc_plain_text2, &cbc_cipher_text2, &key_text, iv_tmp) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }

    if (!memcmp(cbc_plain_text1.buffer, ciph_data_aes_in, cbc_plain_text1.length) &&
        !memcmp(cbc_plain_text2.buffer, (ciph_data_aes_in + cbc_plain_text1.length), cbc_plain_text2.length)) {
        printf("AES CBC iteration decrypt test pass\n");
    } else {
        printf("AES CBC iteration decrypt test fail\n");
        goto FAIL_OUT;
    }

    /* AES GCM mode */
    hal_aes_buffer_t iv_text, aad_text, tag_text;

    iv_text.buffer = gcm_iv;
    iv_text.length = sizeof(gcm_iv);
    aad_text.buffer = gcm_aad;
    aad_text.length = sizeof(gcm_aad);
    tag_text.buffer = gcm_tag;
    tag_text.length = sizeof(gcm_tag);

    /* AES GCM Test key */
    memcpy(key, ciph_gcm_key, sizeof(ciph_gcm_key));
    key_text.buffer = key;
    key_text.length = sizeof(ciph_gcm_key);

    plain_text.buffer = pbuf;
    plain_text.length = sizeof(ciph_data_aes_in_gcm);
    cipher_text.buffer = cbuf;
    cipher_text.length = sizeof(ciph_data_aes_in_gcm);

    /* gcm encryption */
    /* in:  plain_text, content: ciph_data_aes_in_gcm
     * out: cipher_text, content: ciph_data_aes_gcm_out
     */
    memcpy(plain_text.buffer, ciph_data_aes_in_gcm, sizeof(ciph_data_aes_in_gcm));

    if (hal_aes_gcm_encrypt(&cipher_text, &plain_text, &iv_text, &aad_text, &key_text, &tag_text) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(cipher_text.buffer, ciph_data_aes_gcm_out, cipher_text.length) &&
        !memcmp(tag_text.buffer, ciph_gcm_tag_golden, tag_text.length)) {
        printf("AES GCM encryption test pass\n");
    } else {
        printf("AES GCM encryption test fail\n");
        goto FAIL_OUT;
    }

    /* gcm decryption */
    /* in:  cipher_text, content: ciph_data_aes_gcm_out
     * out: plain_text, content: ciph_data_aes_in_gcm
     */
    if (hal_aes_gcm_decrypt(&plain_text, &cipher_text, &iv_text, &aad_text, &key_text, &tag_text) != HAL_AES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(plain_text.buffer, ciph_data_aes_in_gcm, plain_text.length)) {
        printf("AES GCM decryption test pass\n");
    } else {
        printf("AES GCM decryption test fail\n");
        goto FAIL_OUT;
    }

    ci_ret = CI_PASS;

FAIL_OUT:
    free(cbuf);
FAIL_PBUF:
    free(pbuf);
FAIL_GOLDEN:
    free(golden);
FAIL_KEY:
    free(key);

    return ci_ret;
}

ci_status_t ci_gcpu_crypto_des_sample(void)
{
    UINT8 *pbuf, *cbuf, *key;
    hal_des_buffer_t plain_text, cipher_text, key_text;
    UINT32 ci_ret = CI_FAIL;

    key = (UINT8 *)malloc(CIPH_DES_KEY_SIZE);
    if (key == NULL) {
        printf("key buffer alloc failed\n");
        return CI_FAIL;
    }

    pbuf = (UINT8 *)malloc(CIPH_BUFFER_SIZE);
    if (pbuf == NULL) {
        printf("plain_text buffer alloc failed\n");
        goto FAIL_KEY;
    }

    cbuf = (UINT8 *)malloc(CIPH_BUFFER_SIZE);
    if (cbuf == NULL) {
        printf("cipher_text buffer alloc failed\n");
        goto FAIL_PBUF;
    }

    /* DES ECB mode */
    plain_text.buffer = pbuf;
    plain_text.length = sizeof(ciph_data_des_in1);
    cipher_text.buffer = cbuf;
    cipher_text.length = sizeof(ciph_data_des_in1);

    /* ECB key */
    memcpy(key, des_ecb_key, sizeof(des_ecb_key));
    key_text.buffer = key;
    key_text.length = CIPH_DES_KEY_SIZE;

    /* ecb encryption */
    /* in:  plain_text, content: ciph_data_des_in1
     * out: cipher_text, content: ciph_data_des_ecb_out
     */
    memcpy(plain_text.buffer, ciph_data_des_in1, sizeof(ciph_data_des_in1));

    if (hal_des_ecb_encrypt(&cipher_text, &plain_text, &key_text) != HAL_DES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(cipher_text.buffer, ciph_data_des_ecb_out, cipher_text.length)) {
        printf("DES ECB encryption test pass\n");
    } else {
        printf("DES ECB encryption test fail\n");
        goto FAIL_OUT;
    }

    /* ecb decryption */
    /* in:  cipher_text, content: ciph_data_des_ecb_out
     * out: plain_text, content: ciph_data_des_in1
     */
    memset(plain_text.buffer, 0, plain_text.length);

    if (hal_des_ecb_decrypt(&plain_text, &cipher_text, &key_text) != HAL_DES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(plain_text.buffer, ciph_data_des_in1, plain_text.length)) {
        printf("DES ECB decryption test pass\n");
    } else {
        printf("DES ECB decryption test fail\n");
        goto FAIL_OUT;
    }

    /* DES CBC mode */
    plain_text.buffer = pbuf;
    plain_text.length = sizeof(ciph_data_des_in2);
    cipher_text.buffer = cbuf;
    cipher_text.length = sizeof(ciph_data_des_in2);

    /* CBC key */
    memcpy(key, des_cbc_key, sizeof(des_cbc_key));
    key_text.buffer = key;
    key_text.length = CIPH_DES_KEY_SIZE;

    /* cbc encryption */
    /* in:  plain_text, content: ciph_data_des_in2
     * out: cipher_text, content: ciph_data_des_cbc_out
     */
    memcpy(plain_text.buffer, ciph_data_des_in2, sizeof(ciph_data_des_in2));
    if (hal_des_cbc_encrypt(&cipher_text, &plain_text, &key_text, des_iv) != HAL_DES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(cipher_text.buffer, ciph_data_des_cbc_out, cipher_text.length)) {
        printf("DES CBC encryption test pass\n");
    } else {
        printf("DES CBC encryption test fail\n");
        goto FAIL_OUT;
    }

    /* cbc decryption */
    /* in:  cipher_text, content: ciph_data_des_cbc_out
     * out: plain_text, content: ciph_data_des_in2
     */
    memset(plain_text.buffer, 0, plain_text.length);

    if (hal_des_cbc_decrypt(&plain_text, &cipher_text, &key_text, des_iv) != HAL_DES_STATUS_OK) {
        goto FAIL_OUT;
    }
    if (!memcmp(plain_text.buffer, ciph_data_des_in2, plain_text.length)) {
        printf("DES_CBC decryption test pass\n");
    } else {
        printf("DES_CBC decryption test pass\n");
        goto FAIL_OUT;
    }

    ci_ret = CI_PASS;

FAIL_OUT:
    free(cbuf);
FAIL_PBUF:
    free(pbuf);
FAIL_KEY:
    free(key);

    return ci_ret;
}

ci_status_t ci_gcpu_crypto_sha_sample(void)
{
    hal_sha1_context_t sha1_context, sha224_context, sha256_context;
    UINT8 *sha_digest;

    /* SHA1 mode */
    sha_digest = (UINT8 *)malloc(HAL_SHA1_DIGEST_SIZE);
    if (!sha_digest) {
        printf("failed to alloc sha1 digest buffer\n");
        return CI_FAIL;
    }

    hal_sha1_init(&sha1_context);
    hal_sha1_append(&sha1_context, hash_data_sha1_in, sizeof(hash_data_sha1_in));
    hal_sha1_end(&sha1_context, sha_digest);
    if (!memcmp(sha_digest, hash_data_sha1_out, HAL_SHA1_DIGEST_SIZE)) {
        free(sha_digest);
        printf("SHA SHA1 test pass\n");
    } else {
        free(sha_digest);
        printf("SHA SHA1 test fail\n");
        return CI_FAIL;
    }

    /* SHA224 mode */
    sha_digest = (UINT8 *)malloc(HAL_SHA224_DIGEST_SIZE);
    if (!sha_digest) {
        printf("failed to alloc sha224 digest buffer\n");
        return CI_FAIL;
    }

    hal_sha224_init(&sha224_context);
    hal_sha224_append(&sha224_context, hash_data_sha224_in, sizeof(hash_data_sha224_in));
    hal_sha224_end(&sha224_context, sha_digest);
    if (!memcmp(sha_digest, hash_data_sha224_out, HAL_SHA224_DIGEST_SIZE)) {
        free(sha_digest);
        printf("SHA SHA224 test pass\n");
    } else {
        free(sha_digest);
        printf("SHA SHA224 test fail\n");
        return CI_FAIL;
    }

    /* SHA256 mode */
    sha_digest = (UINT8 *)malloc(HAL_SHA256_DIGEST_SIZE);
    if (!sha_digest) {
        printf("failed to alloc sha256 digest buffer\n");
        return CI_FAIL;
    }

    hal_sha256_init(&sha256_context);
    hal_sha256_append(&sha256_context, hash_data_sha256_in, sizeof(hash_data_sha256_in));
    hal_sha256_end(&sha256_context, sha_digest);
    if (!memcmp(sha_digest, hash_data_sha256_out, HAL_SHA256_DIGEST_SIZE)) {
        free(sha_digest);
        printf("SHA SHA256 test pass\n");
    } else {
        free(sha_digest);
        printf("SHA SHA256 test fail\n");
        return CI_FAIL;
    }

    return CI_PASS;
}

ci_status_t ci_gcpu_crypto_md5_sample(void)
{
    hal_md5_context_t md5_context;
    UINT8 *md5_digest;

    /* MD5 mode */
    md5_digest = (UINT8 *)malloc(HAL_MD5_DIGEST_SIZE);
    if (!md5_digest) {
        printf("failed to alloc md5 digest buffer\n");
        return CI_FAIL;
    }

    hal_md5_init(&md5_context);
    hal_md5_append(&md5_context, hash_data_md5_in, sizeof(hash_data_md5_in));
    hal_md5_end(&md5_context, md5_digest);

    if (!memcmp(md5_digest, hash_data_md5_out, HAL_MD5_DIGEST_SIZE)) {
        free(md5_digest);
        printf("MD5 test pass\n");
    } else {
        free(md5_digest);
        printf("MD5 test fail\n");
        return CI_FAIL;
    }

    return CI_PASS;
}


ci_status_t ci_gcpu_sample_main(unsigned int portnum)
{

    struct test_entry test_entry_list[] = {
        {"Sample Code: GCPU crypto aes sample", ci_gcpu_crypto_aes_sample},
        {"Sample Code: GCPU crypto des sample", ci_gcpu_crypto_des_sample},
        {"Sample Code: GCPU crypto sha sample", ci_gcpu_crypto_sha_sample},
        {"Sample Code: GCPU crypto md5 sample", ci_gcpu_crypto_md5_sample},
    };

    return test_execution(test_entry_list, (sizeof(test_entry_list) / sizeof(struct test_entry)));
}
