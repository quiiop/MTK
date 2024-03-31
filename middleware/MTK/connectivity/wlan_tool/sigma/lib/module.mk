include $(SOURCE_DIR)/$(SIGMA_SRC)/Makefile.inc

C_FILES += $(SIGMA_SRC)/mediatek/wpa/wpa_ctrl.c
C_FILES += $(SIGMA_SRC)/mediatek/wpa/wpa_helpers.c
C_FILES += $(SIGMA_SRC)/lib/wfa_miscs.c
C_FILES += $(SIGMA_SRC)/lib/wfa_sock.c
C_FILES += $(SIGMA_SRC)/lib/wfa_tlv.c
C_FILES += $(SIGMA_SRC)/mediatek/mtk_cs.c
C_FILES += $(SIGMA_SRC)/mediatek/mtk_wfd.c
C_FILES += $(SIGMA_SRC)/lib/wfa_cmdtbl.c
C_FILES += $(SIGMA_SRC)/lib/wfa_tg.c
C_FILES += $(SIGMA_SRC)/lib/wfa_thr.c
C_FILES += $(SIGMA_SRC)/lib/wfa_wmmps.c

#include $(SOURCE_DIR)/prebuilt/$(SIGMA_SRC)/module.mk
