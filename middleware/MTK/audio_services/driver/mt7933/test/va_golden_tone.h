#ifndef _GOLDEN_TONE_H_
#define _GOLDEN_TONE_H_

enum {
    VOW_ACCEPT = 0,
    VOW_REJECT,
};
//void va_alarm_play(unsigned int* alarm_index);
void *va_alarm_create(void);
void notify_alarm_play(int value);

#endif /* #ifndef _GOLDEN_TONE_H_ */
