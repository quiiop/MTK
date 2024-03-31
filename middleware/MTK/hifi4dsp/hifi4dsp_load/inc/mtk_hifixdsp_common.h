/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _MTK_HIFIXDSP_COMMON_
#define _MTK_HIFIXDSP_COMMON_

/**
 *@addtogroup HIFI4DSP_LOAD
 *@{
 * @brief This section introduces the hifi4dsp_load APIs including
 * function groups, enums, structures and functions.
 *
 * hifi4dsp_load is a software engine designed to load ADSP(Audio DSP)
 * firmware to ADSP MCU and provide the power on interface to boot up
 * ADSP.
 *
 * @section HIFI4DSP_LOAD_Features_Chapter Supported features
 * - \b Load \b binary \b and \b boot
 *   Load hifi4dsp binary to ADSP and run firmware.
 *
 * - \b Shared \b memory \b between \b AP \b and \b ADSP
 *   Use shared memory to communicate between AP and ADSP and get memory address.
 *
 */

/** @defgroup HIFI4DSP_LOAD_enum Enumeration
  * @{
  */

/** @brief ADSP reserve memory ID definition
 */
enum ADSP_RESERVE_MEM_ID {
    ADSP_A_AUDIO_MEM_ID,
    ADSP_NUMS_SYSRAM_ID,
};

/** @brief Hardware semaphore ID definition.
 * Here, add SEMA-ID definition MUST be
 * sync with the SEMA-ID definition in FreeRTOS DSP project.
 */
enum ADSP_HW_SEMAPHORE_ID {
    ADSP_HW_SEMA_EXAMPLE = 0,
    ADSP_HW_SEMA_IPI = 1,
    /**<  User specified. */
    ADSP_HW_SEMA_MAX = 8
};

/**
  * @}
  */

/** @defgroup HIFI4DSP_LOAD_typedef Typedef
  * @{
  */

/**
 * A function pointer prototype to run a callback function with input argument.
 *
 * @brief User can register a callback function  and this function will be called once ADSP boots up.
 *
 */
typedef void (*callback_fn)(void *arg);
/**
  * @}
  */

/*
 * Public function API for Audio system
 */
/**
 * @brief          This function is used to get ADSP's status, boot up done or not.
 *
 * @return         1: ADSP boots up; 0: ADSP does not boot yet.
 */
extern int hifixdsp_run_status(void);

/**
 * @brief     Load binary to ADSP and starts to run. User can register a function pointer
 *            and this function will be run once ADSP is alive.
 *
 * @return    0 if ADSP runs successfully;
 *            non-zero if any error, please check the error log for more detail.
 * @param[in] callback callback function to run.
 * @param[in] param the input argument for callback function.
 */
extern int async_load_hifixdsp_bin_and_run(callback_fn callback, void *param);

/**
 * @brief     Shutdown ADSP and not run.
 *
 * @return    0 if ADSP stops successfully;
 *            non-zero if any error, please check the error log for more detail.
 */
extern int hifixdsp_stop_run(void);

/**
 * @brief     Get HW semaphore resource.
 *
 * @return    ADSP_HW_SEMA_OK: Success;
 *            ADSP_HW_SEMA_HAVE_GOT: this HW semaphore has been held;
 *            ADSP_HW_SEMA_INVALID: Invalid HW semaphore ID;
 *            ADSP_HW_SEMA_TIMEOUT: timeout
 * @param[in] sema_id semaphore id
 * @param[in] timeout 0 - no timeout; other value -retry count
 */
extern int hw_semaphore_get(enum ADSP_HW_SEMAPHORE_ID sema_id,
                            unsigned int timeout);

/**
 * @brief     Release HW semaphore resource.
 *
 * @return    ADSP_HW_SEMA_OK: Success;
 *            ADSP_HW_SEMA_INVALID: Invalid HW semaphore ID;
 *            ADSP_HW_SEMA_HAVE_GOT: this HW semaphore has been held;
 *            ADSP_HW_SEMA_FREE_ERR: Free Hw semaphore  error
 * @param[in] sema_id semaphore id
 */
extern int hw_semaphore_release(enum ADSP_HW_SEMAPHORE_ID sema_id);

/**
 * @brief     Get reserved sysram physical address with pre-defined ID.
 *
 * @return    Physical address
 * @param[in] id pre-defined ID for reserved memory
 */
extern unsigned long adsp_get_reserve_sysram_phys(enum ADSP_RESERVE_MEM_ID id);

/**
 * @brief     Get reserved sysram virtual address with pre-defined ID.
 *
 * @return    Virtual address
 * @param[in] id pre-defined ID for reserved memory
 */
extern unsigned long adsp_get_reserve_sysram_virt(enum ADSP_RESERVE_MEM_ID id);

/**
 * @brief     Get reserved sysram size with pre-defined ID.
 *
 * @return    Memory size
 * @param[in] id pre-defined ID for reserved memory
 */
extern unsigned long adsp_get_reserve_sysram_size(enum ADSP_RESERVE_MEM_ID id);

/**
 * @brief     Get physical address of shared memory for ipc(inter processor communication).
 *
 * @return    Physical address
 */
extern unsigned long adsp_get_shared_dtcm_phys_for_ipc(void);

/**
 * @brief     Get virtual address of shared memory for ipc(inter processor communication).
 *
 * @return    Virtual address
 */
extern unsigned long adsp_get_shared_dtcm_virt_for_ipc(void);

/**
 * @brief     Get size of shared memory for ipc(inter processor communication).
 *
 * @return    Memory size
 */
extern unsigned long adsp_get_shared_dtcm_size_for_ipc(void);

/**
 * @brief     Get virtual address mapping with given physical address for shared sysram.
 *
 * @return    Virtual address
 * @param[in] addr physical address
 */
extern void *adsp_get_shared_sysram_phys2virt(unsigned long addr);

/**
 * @brief     Get physical address mapping with given virtual address for shared sysram.
 *
 * @return    Physical address
 * @param[in] addr virtual address
 */
extern unsigned long adsp_get_shared_sysram_virt2phys(void *addr);

/**
 * @brief     Get physical address mapping of ADSP view from system view.
 *
 * @return    ADSP view address
 * @param[in] addr system view address
 */
extern unsigned long adsp_hal_phys_addr_cpu2dsp(unsigned long addr);

/**
 * @brief     Get physical address mapping of system view from ADSP view.
 *
 * @return    System view address
 * @param[in] addr ADSP view address
 */
extern unsigned long adsp_hal_phys_addr_dsp2cpu(unsigned long addr);

/**
 * @brief     ADSP driver initialization. Probe device resource.
 *
 * @return    None
 */
extern void adsp_init(void);

/**
 * @brief     ADSP driver uninit. Remove device resource.
 *
 * @return    1
 */
extern int adsp_device_remove(void);

/**
 *@}
 */
#endif /* #ifndef _MTK_HIFIXDSP_COMMON_ */
