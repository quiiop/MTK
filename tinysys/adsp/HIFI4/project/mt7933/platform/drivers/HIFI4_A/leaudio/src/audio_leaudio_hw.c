
#include "audio_leaudio_hw_reg.h"

void clear_connsys_leaudio_interrupt(void)
{
    LEAUDIO_IRQ_CONNSYS_REG |= LEAUDIO_IRQ_CONNSYS_BITMASK ;
}

