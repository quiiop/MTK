
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <hal_sha.h>

#include <v3/fota_migrate.h>


/*
gcc -std=c99 \
    -g -O0 -m32 \
    -DMTK_FOTA_V3_ENABLE \
    -DHAL_FLASH_MODULE_ENABLED \
    -DHAL_SHA_MODULE_ENABLED \
    -Iopenssl-1.0.1g/include \
    -Lopenssl-1.0.1g \
    -I../../../../../driver/CMSIS/Include \
    -I../../../../../driver/CMSIS/Device/MTK/mt7933/Include \
    -I../../../../../project/mt7933_hdk/apps/bga_sdk_demo/inc \
    -I../../../../../driver/chip/mt7933/inc \
    -I../../../../../driver/chip/inc \
    -I. -I../../inc \
    -Wall \
    -o ut ut.c fota_migrate.c fota_api.c fota_log.c -lssl -lcrypto
    
gdb ut
set args image.bin
b main
r

 */


#include <openssl/sha.h>


static SHA_CTX sha_ctx;

hal_sha_status_t hal_sha1_init(hal_sha1_context_t *context)
{
    assert(SHA1_Init(&sha_ctx));
    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha1_append(hal_sha1_context_t *context, const uint8_t *message, uint32_t length)
{
    assert(SHA1_Update(&sha_ctx, message, length));
    return HAL_SHA_STATUS_OK;
}

hal_sha_status_t hal_sha1_end(hal_sha1_context_t *context, uint8_t digest_message[HAL_SHA1_DIGEST_SIZE])
{
    assert(HAL_SHA1_DIGEST_SIZE == SHA_DIGEST_LENGTH);
    assert(SHA1_Final(digest_message, &sha_ctx));
    return HAL_SHA_STATUS_OK;
}


uint8_t *g_flash;

hal_flash_status_t hal_flash_read(uint32_t start_address, uint8_t *buffer, uint32_t length)
{
    memcpy(buffer, g_flash + start_address, length);
    return HAL_FLASH_STATUS_OK;
}

hal_flash_status_t hal_flash_erase(uint32_t start_address,  hal_flash_block_t block_type)
{
    int kb_4 = 1 << 12;

    assert (block_type == HAL_FLASH_BLOCK_4K);
    assert ((start_address & (kb_4 - 1)) == 0);
    memset(g_flash + start_address, 0, kb_4);
    return HAL_FLASH_STATUS_OK;
}

hal_flash_status_t hal_flash_write(uint32_t address, const uint8_t *data, uint32_t length)
{
    memcpy(g_flash + address, data, length);
    return HAL_FLASH_STATUS_OK;
}

void *fota_malloc( size_t size )
{
    if ( size > 0 )
    {
        return malloc( size );
    }
    return NULL;
}

void fota_free( void *ptr )
{
    free( ptr );
}


int main(int argc, char *argv[])
{
    if (argc <= 1)
        return -1;
    
    assert(g_flash = malloc( 16 * 1024 * 1024 ));

    for (int i = 1; i < argc; i++)
    {
        int fd = open(argv[i], O_RDONLY);
        
        if (fd < 0)
            return -2;

        int nbyte = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);

        void *buf = malloc(nbyte);
        if (!buf)
            return -3;
            
        if (read(fd, buf, nbyte) < nbyte)
            return -4;

        fota_status_t status;
        status = fota_migrate_mem(buf, nbyte);
        printf("status %u\n", status);
        
        free(buf);
        close(fd);
    }
    
    return 0;
}
