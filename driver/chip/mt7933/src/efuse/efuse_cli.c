/*
 * (C) 2005-2020 MediaTek Inc. All rights reserved.
 *
 * Copyright Statement:
 *
 * This MT7933 driver software/firmware and related documentation
 * ("MediaTek Software") are protected under relevant copyright laws.
 * The information contained herein is confidential and proprietary to
 * MediaTek Inc. ("MediaTek"). You may only use, reproduce, modify, or
 * distribute (as applicable) MediaTek Software if you have agreed to and been
 * bound by this Statement and the applicable license agreement with MediaTek
 * ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User"). If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
 * PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS
 * ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR
 * NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH
 * RESPECT TO THE SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY,
 * INCORPORATED IN, OR SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO
 * LOOK ONLY TO SUCH THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO.
 * RECEIVER EXPRESSLY ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO
 * OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK
 * SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE
 * RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S
 * ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED
 * HEREUNDER WILL BE ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
 * RECEIVER TO MEDIATEK DURING THE PRECEDING TWELVE (12) MONTHS FOR SUCH
 * MEDIATEK SOFTWARE AT ISSUE.
 */

#include <cli.h>
#include <common.h>
#include <ctype.h>
#include <efuse_cli.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum EEF_GRP {
    GROUP_1ST,
    GROUP_2ND,
    GROUP_3ND,
};

enum efuse_test_cmd {
    EFUSE_READ = 0,
    EFUSE_WRITE,
    EFUSE_CMD_NUM
};

static uint8_t cli_physical_ereader(uint8_t len, char *param[])
{
    unsigned int uEfuseAddr, uEfuseGroup;
    int ret = 0, i = 0;
    long unsigned int efuse_buf[4];

    if (len != 2) {
        MSG("invalid arguments\r\n");
        ret = -1;
        goto out;
    }

    /* Obtain the parameter string. */
    uEfuseGroup = strtoul(param[0], NULL, 0);
    uEfuseAddr = strtoul(param[1], NULL, 16);

    if (uEfuseGroup > EFUSE_MAX_GROUP) {
        MSG("group is out of bound\r\n");
        ret = -1;
        goto out;
    }

    ret = efuse_physical_read(uEfuseGroup, uEfuseAddr, efuse_buf);

    if (ret) {
        MSG("failed to read efuse\r\n");
    } else {
        MSG("efuse hex: ");
        for (i = 0; i < 4; i++)
            MSG("%#lx ", *(efuse_buf + i));
        MSG("\r\n");
    }
out:
    return ret;
}


static uint8_t cli_logical_ereader(uint8_t len, char *param[])
{
    unsigned int uEfuseAddr, uEfuseGroup;
    int ret = 0, i = 0;
    long unsigned int efuse_buf[4];

    if (len != 2) {
        MSG("invalid arguments\r\n");
        ret = -1;
        goto out;
    }

    /* Obtain the parameter string. */
    uEfuseGroup = strtoul(param[0], NULL, 0);
    uEfuseAddr = strtoul(param[1], NULL, 16);

    if (uEfuseGroup > EFUSE_MAX_GROUP || uEfuseGroup == 0) {
        MSG("group is out of bound\r\n");
        ret = -1;
        goto out;
    }

    ret = efuse_logical_read(uEfuseGroup, uEfuseAddr, efuse_buf);

    if (ret) {
        MSG("failed to read efuse\r\n");
    } else {
        MSG("efuse hex: ");
        for (i = 0; i < 4; i++)
            MSG("%08lx ", *(efuse_buf + i));
        MSG("\r\n");
    }
out:
    return ret;
}

static uint8_t cli_physical_ewriter(uint8_t len, char *param[])
{
    unsigned int uEfuseAddr, uEfuseGroup;
    uint32_t efuse_value[4];
    int ret = 0, i = 0;

    if (len != 6) {
        MSG("cli_ewriter: invalid arguments\r\n");
        ret = -1;
        goto out;
    }

    /* Obtain the parameter string. */
    uEfuseGroup = strtoul(param[0], NULL, 0);
    uEfuseAddr = strtoul(param[1], NULL, 16);

    if (uEfuseGroup > EFUSE_MAX_GROUP) {
        MSG("group is out of bound\r\n");
        ret = -1;
        goto out;
    }

    if (uEfuseGroup == GROUP_2ND || uEfuseGroup == GROUP_3ND) {
        MSG("ewriter only for group1\r\n");
        ret = -1;
        goto out;
    }

    for (i = 0; i < 4 ; i++) {
        efuse_value[i] = strtoul(param[2 + i], NULL, 16);
    }

    ret = efuse_physical_write(uEfuseGroup, uEfuseAddr, efuse_value);

    if (ret) {
        MSG("failed to write efuse status:%d \r\n", ret);
        ret = -1;
        goto out;
    }

    MSG("Blow Ok\r\n");
    MSG("please use efusedrv ereader_group to check blow success\r\n");

out:
    return ret;
}

static uint8_t cli_physical_ereader_group(uint8_t len, char *param[])
{
    unsigned int uEfuseGroup;
    int ret = 0;
    long unsigned int efuse_buf[4];
    long unsigned int efuse_bound[3] = {0x200, 0x200, 0x400}; // grp1 512 bytes grp2 512 bytes grp3 1k bytes

    if (len != 1) {
        MSG("invalid arguments\r\n");
        ret = -1;
        goto out;
    }

    /* Obtain the parameter string. */
    uEfuseGroup = strtoul(param[0], NULL, 0);

    if (uEfuseGroup > EFUSE_MAX_GROUP) {
        MSG("group is out of bound\r\n");
        ret = -1;
        goto out;
    }

    for (long unsigned int i = 0; i < efuse_bound[uEfuseGroup]; i = i + 0x10) {
        ret = efuse_physical_read(uEfuseGroup, i, efuse_buf);

        if (ret) {
            MSG("failed to read efuse\r\n");
        } else {
            MSG("offset(%08lx) efuse hex:", i);
            for (int j = 0; j < 4; j++)
                MSG("%08lx ", *(efuse_buf + j));
            MSG("\r\n");
        }
    }
out:
    return ret;
}

static uint8_t cli_ewriter(uint8_t len, char *param[])
{
    unsigned int cmd, index, length, reg_cnt, i;
    unsigned int reg_size = 4;
    int ret = 0;
    uint32_t efuse_value[4] = {0};

    cmd = strtoul(param[0], NULL, 0);

    length = strtoul(param[2], NULL, 0);
    switch (cmd) {
        case EFUSE_READ:
            if (len != 3) {
                MSG("invalid arguments\r\n");
                ret = -1;
                goto out;
            }
            index = strtoul(param[1], NULL, 0);

            ret = ewriter_fuse_read(index, length, efuse_value);
            if (ret) {
                MSG("failed to read efuse status:%d \r\n", ret);
                ret = -1;
                goto out;
            }
            MSG("efuse index:%d length:%d\r\n", index, length);
            for (i = 0; i < 4 ; i++) {
                MSG("efuse_value[%d]:%lx\r\n", i, efuse_value[i]);
            }
            break;

        case EFUSE_WRITE:
            if (len == 4 || len == 7) {
                index = strtoul(param[1], NULL, 0);

                reg_cnt = length < reg_size ? 1 : (length >> 2);
                for (i = 0; i < reg_cnt ; i++) {
                    efuse_value[i] = strtoul(param[3 + i], NULL, 16);
                }

                ret = ewriter_fuse_write(index, length, efuse_value);
                if (ret) {
                    MSG("failed to write efuse status:%d \r\n", ret);
                    ret = -1;
                    goto out;
                }
                MSG("Blow Ok\r\n");
            } else {
                MSG("invalid arguments\r\n");
                ret = -1;
                goto out;
            }
            break;

        default:
            MSG("ewriter default\r\n");
    }
out:
    return ret;
}

const cmd_t efuse_driver_cli[] = {
    {"ewriter_phy", "cli_command_ewriter <group> <offset(hex)> <value(hex)> <value(hex)> <value(hex)> <value(hex)>", cli_physical_ewriter, NULL},
    {"ereader_phy", "cli_command_ereader <group> <offset(hex)>", cli_physical_ereader, NULL},
    {"ereader_log", "cli_command_ereader <group> <offset(hex)>", cli_logical_ereader, NULL},
    {"ereader_group", "cli_command_ereader <group>", cli_physical_ereader_group, NULL},
    {"ewriter_index", "cli ewriter with index <read/write> <index> <length> <value(hex)> <value(hex)> <value(hex)> <value(hex)>(writing only)>", cli_ewriter, NULL},
    {NULL, NULL, NULL, NULL }
};
