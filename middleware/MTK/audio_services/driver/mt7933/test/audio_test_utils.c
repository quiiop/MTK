#include <string.h>
#include <stdio.h>
#include <math.h>

#include "audio_test_utils.h"
#include "sound/include/tinycontrol.h"
#include "sound/utils/include/aud_log.h"
#include "sound/driver/include/errno.h"

#ifdef FREERTOS_POSIX_SUPPORT
#include "freertos/snd_portable.h"

static pthread_mutex_t msg_mutex;
static pthread_once_t once_config = PTHREAD_ONCE_INIT;

static void snd_control_lock_init(void)
{
    int ret = 0;
    ret = pthread_mutex_init(&msg_mutex, NULL);
    if (ret)
        aud_error("pthread_mutex_init error: mgr_mutex");
}

static inline void snd_control_lock(void)
{
    pthread_once(&once_config, snd_control_lock_init);
    pthread_mutex_lock(&msg_mutex);
}

void snd_control_unlock(void)
{
    pthread_mutex_unlock(&msg_mutex);
}

#else /* #ifdef FREERTOS_POSIX_SUPPORT */

static inline void snd_control_lock(void) {}
static inline void snd_control_unlock(void) {}

#endif /* #ifdef FREERTOS_POSIX_SUPPORT */


// type = CONNECT_FE_BE
// type = CONNECT_IO_PORT
int connect_route(const char *src, const char *sink, int value, int type)
{
    int ret = 0;
    sound_t *snd = NULL;
    struct route r;
    struct msd_ctl_value ctl_value;

    r.src = src;
    r.sink = sink;
    r.value = value;
    memset(&ctl_value, 0, sizeof(ctl_value));
    ctl_value.ctl_id.id = type;
    strncpy(ctl_value.ctl_id.name, "io_route_ctl", MSD_CTL_ID_NAME_MAX_LEN - 1);
    memcpy(ctl_value.bytes.data, &r, sizeof(struct route));

    snd_control_lock();

    ret = snd_ctl_open(&snd, "control");
    if (ret < 0) {
        aud_error("connect_route fail %d.\n", ret);
        snd_control_unlock();
        return ret;
    }
    snd_ctl_write(snd, &ctl_value);
    snd_ctl_close(snd);

    snd_control_unlock();
    return 0;
}

int control_cget(const char *control_name, int value_count)
{
    sound_t *snd = NULL;
    int index;
    int ret = 0;

    if (!control_name) {
        aud_error("control_name is NULL");
        return -EINVAL;
    }

    struct msd_ctl_value ctl_value;
    memset(&ctl_value, 0, sizeof(ctl_value));
    strncpy(ctl_value.ctl_id.name, control_name, MSD_CTL_ID_NAME_MAX_LEN - 1);

    snd_control_lock();

    ret = snd_ctl_open(&snd, "control");
    if (ret < 0) {
        aud_error("snd_ctl_open fail %d.", ret);
        snd_control_unlock();
        return ret;
    }

    snd_ctl_read(snd, &ctl_value);
    snd_ctl_close(snd);

    snd_control_unlock();

    printf("%s :\t", control_name);
    for (index = 0; index < value_count; index ++) {
        printf("%d\t", ctl_value.integer.value[index]);
    }
    printf("\r\n");
    return 0;
}

int control_cget_v2(const char *control_name, int value_count, void *out_value)
{
    sound_t *snd = NULL;
    int index;
    int ret = 0;

    if (!control_name) {
        aud_error("control_name is NULL");
        return -EINVAL;
    }

    int *value_p = (int *)out_value;

    struct msd_ctl_value ctl_value;
    memset(&ctl_value, 0, sizeof(ctl_value));
    strncpy(ctl_value.ctl_id.name, control_name, MSD_CTL_ID_NAME_MAX_LEN - 1);

    snd_control_lock();

    ret = snd_ctl_open(&snd, "control");
    if (ret < 0) {
        aud_error("snd_ctl_open fail %d.", ret);
        snd_control_unlock();
        return ret;
    }

    snd_ctl_read(snd, &ctl_value);
    snd_ctl_close(snd);

    snd_control_unlock();

    for (index = 0; index < value_count; index ++) {
        value_p[index] = ctl_value.integer.value[index];
    }

    return 0;
}

int control_cset(const char *control_name, int value_count, void *value)
{
    sound_t *snd = NULL;
    int index;
    int ret = 0;

    if (!control_name) {
        aud_error("control_name is NULL");
        return -EINVAL;
    }

    int *value_p = (int *)value;

    struct msd_ctl_value ctl_value;
    memset(&ctl_value, 0, sizeof(ctl_value));
    strncpy(ctl_value.ctl_id.name, control_name, MSD_CTL_ID_NAME_MAX_LEN - 1);
    for (index = 0; index < value_count; index ++) {
        ctl_value.integer.value[index] = value_p[index];
    }

    snd_control_lock();

    ret = snd_ctl_open(&snd, "control");
    if (ret < 0) {
        aud_error("snd_ctl_open fail %d", ret);
        snd_control_unlock();
        return ret;
    }

    ret = snd_ctl_write(snd, &ctl_value);
    if (ret < 0) {
        aud_error("snd_ctl_write fail %d", ret);
        snd_control_unlock();
        return ret;
    }
    snd_ctl_close(snd);

    snd_control_unlock();
    return 0;
}

void volatile_memcpy(volatile void *dest, volatile void *src, unsigned int size)
{
    //must be 32bit align
    //configASSERT(size%4==0);

    volatile unsigned int *dest_u32 = dest;
    volatile unsigned int *src_u32 = src;

    while (size) {
        *dest_u32 = *src_u32;
        dest_u32++;
        src_u32++;
        size -= 4;
    }
}


unsigned int count_zero_frame(const void *data, unsigned int frames, int ch, bool hdflag)
{
    unsigned int zero = 0;
    int channel = ch;

    while (zero < frames) {
        while (channel--) {
            if (!hdflag) { /* 16bit */
                if ((*(unsigned short *)data & 0xFFFF) != 0x0)
                    return zero;
                data += 2;
            } else { /* 32bit */
                if ((*(unsigned int *)data & 0xFFFFFFFF) != 0x0)
                    return zero;
                data += 4;
            }
        }
        if (channel < 0) {
            zero++;
            channel = ch;
        }
    }

    return zero;
}

#define PI              3.1415926535
void DIF_FFT(Complex x[], unsigned int Nu)
{
    unsigned int i, j, k, ip, I;
    unsigned int N, LE, LE1, Nv2;
    Complex Wn, W, t, temp;
    N = 1;
    N <<= Nu;
    LE = N * 2;
    for (i = 1; i <= Nu; i++) {// the butterfly part
        LE /= 2;
        LE1 = LE / 2;
        Wn.real = 1.0;
        Wn.image = 0; // Wn(0)
        W.real = (double)cos(PI / (double)LE1);
        W.image = (double) - sin(PI / (double)LE1); // Step of Wn increment
        for (j = 1; j <= LE1; j++) {
            for (k = j; k <= N; k += LE) {
                I = k - 1; // index of upper part of butterfly
                ip = I + LE1; // index of lower part of butterfly
                t.real = x[I].real + x[ip].real;
                t.image = x[I].image + x[ip].image; // the output of butterfly upper part
                temp.real = x[I].real - x[ip].real;
                temp.image = x[I].image - x[ip].image; // the output of butterfly lower part
                x[ip].real = temp.real * Wn.real - temp.image * Wn.image; // lower part has to multiply with Wn(k)
                x[ip].image = temp.real * Wn.image + temp.image * Wn.real;
                x[I].real = t.real;
                x[I].image = t.image; // copy t to x[i] directly
            }
            temp.real = W.real * Wn.real - W.image * Wn.image; // Increment Wn(j) to Wn(j+LE)
            Wn.image = W.real * Wn.image + W.image * Wn.real;
            Wn.real = temp.real;
        }
    }
    Nv2 = N / 2;
    j = 1;
    for (i = 1; i <= N - 1; i++) {// bit-reverse
        if (i < j) {
            t.real = x[j - 1].real;
            x[j - 1].real = x[i - 1].real;
            x[i - 1].real = t.real;
            t.image = x[j - 1].image;
            x[j - 1].image = x[i - 1].image;
            x[i - 1].image = t.image;
        }
        k = Nv2;
        while (k < j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }
}

int frequency_check(void *data, unsigned int data_size, unsigned int bitdepth, unsigned int channel_num,
                    unsigned int *frequency, unsigned int sample_rate)
{
    int ret = 0;
    unsigned int frame_size = channel_num * bitdepth / 8;
    unsigned int data_frame = data_size / frame_size;
    unsigned int hdflg = bitdepth == 16 ? 0 : 1;
    void *data_temp;
    unsigned int zero_frame;
    unsigned int index, channel_index;
    Complex *fre_data = NULL;

    /* skip zero frame ahead of valid frames */
    zero_frame = count_zero_frame(data, data_frame, channel_num, hdflg);
    data_temp = data + zero_frame * frame_size;
    data_frame = data_frame - zero_frame;
    aud_msg("%d frames of zero at the beginning of data 0x%x", zero_frame, data_temp);

    if (data_frame <= 0) {
        aud_msg("All zero!");
        return -1;
    }

    unsigned int data_power = 0;
    while ((unsigned int)(1 << data_power) < data_frame) {
        data_power++;
    }
    data_power -= 1;

    aud_msg("data_power = %d, wanted_size = %d", data_power, (unsigned int)(1 << data_power) * sizeof(Complex));

    unsigned int data_len = 1 << data_power;

    unsigned int wanted_size = data_len * sizeof(Complex);
    fre_data = (Complex *)malloc(wanted_size);
    if (!fre_data) {
        aud_error("cannot malloc fre_data!");
        return -1;
    }

    unsigned int *golden_32bit = NULL;
    unsigned short *golden_16bit = NULL;

    if (bitdepth == 16) {
        golden_16bit = (unsigned short *)data_temp;
    } else {
        golden_32bit = (unsigned int *)data_temp;
    }

    for (channel_index = 0; channel_index < channel_num; channel_index++) {
        for (index = 0; index < data_len; index++) {
            if (bitdepth == 16)
                fre_data[index].real = golden_16bit[channel_num * index + channel_index];
            else
                fre_data[index].real = golden_32bit[channel_num * index + channel_index];
            fre_data[index].image = 0;
        }

        DIF_FFT(fre_data, data_power);

        unsigned int max_index = 0;
        double max_data = 0;
        for (index = 10; index < (data_len / 2); index++) {
            double image = fre_data[index].image;
            double real = fre_data[index].real;
            double temp =  image * image + real * real;
            if (temp > max_data) {
                max_index = index;
                max_data = temp;
            }
        }

        unsigned int fre = sample_rate * max_index / data_len;
        aud_msg("sample_rate = %d, data_index = %d, data_power = %d, data_len = %d, frequecy = %d",
                sample_rate, max_index, data_power, data_len, fre);
        aud_msg("frequency[%d] = %d", channel_index, frequency[channel_index]);
        if (fre <= (frequency[channel_index] + 5) && fre >= (frequency[channel_index] - 5)) {
            ret |= 0;
            aud_msg("%d channel check PASS!", channel_index);
        } else {
            ret |= -1;
            aud_msg("%d channel check FAIL!", channel_index);
        }
    }

    if (!ret)
        aud_msg("frequency check PASS!");
    else
        aud_msg("frequency check FAIL!");

    free(fre_data);

    return ret;
}


