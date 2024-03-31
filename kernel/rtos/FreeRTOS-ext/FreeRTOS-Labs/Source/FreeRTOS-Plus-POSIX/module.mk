
###################################################
# Sources
###################################################

POSIX_DIR = kernel/rtos/FreeRTOS-ext/FreeRTOS-Labs/Source/FreeRTOS-Plus-POSIX
POSIX_SRC = $(POSIX_DIR)/FreeRTOS-Plus-POSIX/source
POSIX_TST = $(POSIX_DIR)/FreeRTOS-Plus-POSIX/test


POSIX_FILES  = $(POSIX_SRC)/FreeRTOS_POSIX_clock.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_mqueue.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_pthread_barrier.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_pthread.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_pthread_cond.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_pthread_mutex.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_sched.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_semaphore.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_timer.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_unistd.c
POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_utils.c

POSIX_FILES += $(POSIX_SRC)/FreeRTOS_POSIX_pthread_local_storage.c

#POSIX_FILES += $(POSIX_TST)/posix_test.c

C_FILES += $(POSIX_FILES)

###################################################
# include path
###################################################
CFLAGS  += -I$(SOURCE_DIR)/$(POSIX_DIR)/include
CFLAGS  += -I$(SOURCE_DIR)/$(POSIX_DIR)/include/private
CFLAGS  += -I$(SOURCE_DIR)/$(POSIX_DIR)/FreeRTOS-Plus-POSIX/include
CFLAGS  += -I$(SOURCE_DIR)/$(POSIX_DIR)/FreeRTOS-Plus-POSIX/include/portable
CFLAGS  += -I$(SOURCE_DIR)/$(POSIX_DIR)/FreeRTOS-Plus-POSIX/include/portable/mediatek

#pthread def in toolchain may cause conflict build error, so disable the def in toolchain.
CFLAGS  += -D_SYS__PTHREADTYPES_H_