

#define LEAUDIO_IRQ_REG_BASE         (0x60830000)
#define LEAUDIO_IRQ_CONNSYS_REG      (*(volatile unsigned int *)(LEAUDIO_IRQ_REG_BASE + 0x230))
#define LEAUDIO_ACI_CTRL_DATA_ADDR   (*(volatile unsigned int *)(LEAUDIO_IRQ_REG_BASE + 0x234))
#define LEAUDIO_IRQ_CONNSYS_BITMASK  (1 << 31)

#define BTSRAM_REGION_1 (0x00400000)
#define BTSRAM_INFRA_MAPPING (0x60840000)


void clear_connsys_leaudio_interrupt(void);


