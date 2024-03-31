/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein is
 * confidential and proprietary to MediaTek Inc. and/or its licensors. Without
 * the prior written permission of MediaTek inc. and/or its licensors, any
 * reproduction, modification, use or disclosure of MediaTek Software, and
 * information contained herein, in whole or in part, shall be strictly
 * prohibited.
 *
 * MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER
 * ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES
 * TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE
 * RELEASED HEREUNDER WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE
 * MEDIATEK SOFTWARE AT ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE
 * CHARGE PAID BY RECEIVER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

 
/*!
 *@file   Source.c
 *@brief  defines the api of source interface
 *
 @verbatim
 @endverbatim
 */

//-
#include "source_inter.h"
#include "stream.h"

////////////////////////////////////////////////////////////////////////////////
// Constant Definitions ////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Global Variables ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
SOURCE Source_blks[SOURCE_TYPE_MAX] = {0};

/* 2k byte map  */
U8 MapAddr[4];   // beware of this !!!


////////////////////////////////////////////////////////////////////////////////
// Function Declarations ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reports the number of bytes available in the source.
 *
 * @param source The source to fetch the size of.
 * @return Zero if the source is invalid.
 */
ATTR_TEXT_IN_IRAM_LEVEL_1 U32 SourceSize(SOURCE source)
{
    U32 Size = 0;    
    
    if (source)
    {
        Size = source->sif.SourceSize(source);
    }
    
    return Size;
}

/**
 * @brief copy data in source buffer to map, only number of SourceSize report is valid
 *  data in map become invalid if another sourcemap is called
 *
 * @param source The source to map into the address map.
 *
 * @return address of map, zero if the source is invalid.
 */

U8* SourceMap(SOURCE source)
{
    U8* Map = NULL;
    
    if (source)
    {
        Map = source->sif.SourceMap(source);
    }
    
    return Map;
}


/**
 * @brief drop data in front of source buffer
 *
 * @param source The Source to drop the data from.
 * @param amount The number of bytes to drop.
 */
ATTR_TEXT_IN_IRAM_LEVEL_1 VOID SourceDrop(SOURCE source, U32 amount)
{
    if (source)
    {
        source->sif.SourceDrop(source, amount);
    }  
}

/**
 * @brief Configure a specific source.
 *
 * @param source The source to configure.
 * @param key The key to configure.
 * @param value The value to write to 'key'
 *
 * @return FALSE if the request could not be performed, TRUE otherwise.
 */
BOOL SourceConfigure(SOURCE source, stream_config_type type, U32 value)
{
    BOOL result = FALSE;
    
    if (source)
    {
        result = source->sif.SourceConfigure(source, type, value);
    }
    
    return result;
}

/**
 * @brief Request to close the source
 * @param source The source to close
 *
 * @return TRUE if the source could be closed, and FALSE
 * otherwise.
 */
BOOL SourceClose(SOURCE source)
{
    BOOL result = TRUE;
    
    if (source)
    {
        if (source->transform != NULL)
        {
            StreamDisconnect(source->transform);
        }
        
        if (source->sif.SourceClose != NULL )
        {
            result = source->sif.SourceClose(source);
        }
        if (result)
        {
            Source_blks[source->type] = NULL;
            vPortFree(source);
        }
    }
    return result;
}

/**
 * @brief Return TRUE if a source is valid, FALSE otherwise.
 *
 * @param source The source to check.
 * @return TRUE stands for valid , FALSE otherwise
 */
BOOL SourceIsValid(SOURCE source)
{
    BOOL result = FALSE;
    
    if (source)
    {
        if(source == Source_blks[source->type])
        {
            result = TRUE;
        }
    }

    return result;
}

/**
 * @brief clean buffer in source
 *
 * @param source  the source to clean
 */
VOID SourceEmpty(SOURCE source)
{
    SourceDrop(source,SourceSize(source));
}

/**
 * @brief read buffer in source into destination
 *
 * @param source The source to read.
 * @param dst_addr The destination address to store the data.
 * @param length  The length to read from source and store into destination.
 */
ATTR_TEXT_IN_IRAM_LEVEL_1 BOOL SourceReadBuf(SOURCE source, U8 *dst_addr, U32 length)
{
    BOOL result = FALSE;
    if (source)
    {
        result = source->sif.SourceReadBuf(source, dst_addr, length);
    }
    return result;
}

U8* SourceMapBufType(SOURCE source)
{
    U32 writeOffset = source->streamBuffer.BufferInfo.WriteOffset;
    U32 readOffset  = source->streamBuffer.BufferInfo.ReadOffset;
    U32 length      = source->streamBuffer.BufferInfo.length;

    U32 residue     = length - readOffset;
    U32 MoveLength  = writeOffset >= readOffset ? (writeOffset - readOffset) : (length - readOffset + writeOffset);

    /* source buffer wrap */
    if(MoveLength > residue)
    {
        U32 len1 = residue;
        U32 len2 = MoveLength - len1;
        memcpy(MapAddr,source->streamBuffer.BufferInfo.startaddr[0]+readOffset,len1);
        memcpy(MapAddr+len1,source->streamBuffer.BufferInfo.startaddr[0],len2);
    }
    /* source buffer no wrap */
    else
    {
        memcpy(MapAddr,source->streamBuffer.BufferInfo.startaddr[0]+readOffset,MoveLength);
    }

    return MapAddr;
}

