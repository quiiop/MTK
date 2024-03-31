include $(SOURCE_DIR)/$(SIGMA_SRC)/Makefile.inc

$(info sigma dut $(SIGMA_SRC))
C_FILES += $(SIGMA_SRC)/dut/wfa_dut_freertos.c
C_FILES += $(SIGMA_SRC)/dut/wfa_dut_init.c
