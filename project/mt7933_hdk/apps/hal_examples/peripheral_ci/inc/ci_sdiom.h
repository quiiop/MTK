#ifndef __CI_SDIOM__
#define  __CI_SDIOM__

#define     GPD_BUF_LEN    0x1000

#define     SDIO_BLK_SIZE     256
#define     SDIO_CHIPID_CR    0x00000000

#define     SDIO_IP_WHLPCR  0x00000004
#define     FW_OWN_REQ_CLR  0x00000200
#define     DRV_OWN_STATUS  0x00000100
#define     W_INT_EN_SET    0x00000001

#define     SDIO_IP_WHISR   0x00000010
#define     TX_DONE_INT     0x00000001
#define     RX0_DONE_INT    0X00000002
#define     RX1_DONE_INT    0X00000004

#define     SDIO_IP_WHIER   0x00000014

#define     SDIO_IP_WSICR   0x00000024


#define     SDIO_IP_WTSR0   0x00000028
#define     SDIO_TQ1        0x0000ff00

#define     SDIO_IP_WTDR1   0x00000034

#define     SDIO_IP_WRDR0   0x00000050
#define     SDIO_IP_WRDR1   0x00000054

#define     SDIO_IP_H2DSM0R 0x00000070
#define     SDIO_IP_D2HRM0R 0x00000078

#define     SDIO_IP_WRPLR   0X00000090
#define     SDIO_RX0_LEN    0x0000ffff
#define     SDIO_RX1_LEN    0xffff0000

#define     SDIO_IP_H2DSM0R 0x00000070
#define     MAGIC_CODE_SUCCESS     0x43434343
#define     MAGIC_CODE_FAIL        0x41414141
#define     MAGIC_RX0_FAIL        0x11111111
#define     MAGIC_RX1_FAIL        0x22222222
#define     MAGIC_TX1_FAIL        0x33333333
#define     MAGIC_INT_FAIL        0x44444444

typedef struct SDIO_Host_Data {
    uint32_t tx_len;
    uint32_t data[1023];
} Host_Data;



#endif /* #ifndef __CI_SDIOM__ */
