#ifndef __MT7933_CODEC_H__
#define __MT7933_CODEC_H__

#include "mt7933-reg.h"

#define EEF_GRP2_REG_BASE   (0x00000000)
#define EEF_GRP2_REG_SIZE   (0x200)

/* EEF_GRP2 */
#define AUDADC0_RC_TRIM_L           0x95
#define AUDADC0_RC_TRIM_R           0x96
#define AUDADC1_RC_TRIM_L           0x97
#define AUDADC_LDO_VOSEL            0x98
#define AUDADC0_VREF_VCM1P4_SEL     0x99
#define AUDADC1_VREF_VCM1P4_SEL     0x9A
#define AUD_DAC                     0x9B

/* AUD_DAC */
#define RG_AUD_DAC_VALID_MASK                           BIT(0)
#define AUD_DAC_RG_LDO1P8_ADAC_VOSEL_MASK               GENMASK(7, 4)

#define APMIXED_REG_BASE        (0x30010000)
#define APMIXED_REG_SIZE        (0x1000)

/* apmixedsys */
#define AUD_ADC0_COMMON     0x000
#define AUD_ADC0_RSVD       0x004
#define AUD_ADC0_LCH_CON00  0x010
#define AUD_ADC0_LCH_CON01  0x014
#define AUD_ADC0_LCH_CON02  0x018
#define AUD_ADC0_LCH_CON03  0x01C
#define AUD_ADC0_LCH_CON04  0x020
#define AUD_ADC0_LCH_CON05  0x024
#define AUD_ADC0_RCH_CON00  0x050
#define AUD_ADC0_RCH_CON01  0x054
#define AUD_ADC0_RCH_CON02  0x058
#define AUD_ADC0_RCH_CON03  0x05C
#define AUD_ADC0_RCH_CON04  0x060
#define AUD_ADC0_RCH_CON05  0x064

#define AUD_ADC1_COMMON     0x100
#define AUD_ADC1_RSVD       0x104
#define AUD_ADC1_LCH_CON00  0x110
#define AUD_ADC1_LCH_CON01  0x114
#define AUD_ADC1_LCH_CON02  0x118
#define AUD_ADC1_LCH_CON03  0x11C
#define AUD_ADC1_LCH_CON04  0x120
#define AUD_ADC1_LCH_CON05  0x124
#define AUD_ADC1_RCH_CON00  0x150
#define AUD_ADC1_RCH_CON01  0x154
#define AUD_ADC1_RCH_CON02  0x158
#define AUD_ADC1_RCH_CON03  0x15C
#define AUD_ADC1_RCH_CON04  0x160
#define AUD_ADC1_RCH_CON05  0x164

#define AUD_DAC_CON00       0x200
#define AUD_DAC_CON01       0x204
#define AUD_DAC_CON02       0x208
#define AUD_DAC_CON03       0x20C
#define AUD_DAC_CON04       0x210
#define AUD_DAC_CON05       0x214
#define AUD_ANAREG_CON      0x300
#define XTAL_CLK_CON        0x310


/* AUD_DAC_CON00 */
#define RG_IDACR_PWDB_MASK                          BIT(0)
#define RG_IDACL_PWDB_MASK                          BIT(1)
#define RG_RELATCH_EN_MASK                          BIT(2)
#define RG_V2I_PWDB_MASK                            BIT(7)
#define RG_VMID_RSEL_MASK                           GENMASK(9, 8)
#define RG_VMID_RSEL_VAL(x)                         (((x) & 0x3) << 8)
#define RG_VMID_FASTUP_EN_MASK                      BIT(10)
#define RG_VMID_PWDB_MASK                           BIT(11)
#define RG_GLBIAS_ADAC_EN_MASK                      BIT(12)
#define RG_CLK_INV_MASK                             BIT(13)
#define RG_CLK26MHZ_DIV4_EN_MASK                    BIT(18)
#define RG_CLK26MHZ_EN_MASK                         BIT(19)
#define RG_CK_6P5M_FIFO_EN_MASK                     BIT(20)
#define RG_LDO1P8_ADAC_VOSEL_MASK                   GENMASK(27, 24)
#define RG_LDO1P8_ADAC_VOSEL_VAL(x)                 (((x) & 0xf) << 24)
#define RG_LDO1P8_ADAC_EN_MASK                      BIT(28)

/* AUD_DAC_CON01 */
#define RG_VCMB_SEL_MASK                            BIT(0)
#define RG_EN_VCMBUF_MASK                           BIT(1)
#define RG_I2VR_PWDB_MASK                           BIT(9)
#define RG_I2VL_PWDB_MASK                           BIT(15)

/* AUD_DAC_CON02 */
#define RG_ENVO_MASK                                BIT(14)
#define RG_ENDP_MASK                                BIT(15)
#define RG_DP_S1_MASK                               BIT(16)
#define RG_DP_S0_MASK                               BIT(17)

/* AUD_DAC_CON03 */
#define RG_DEPOP_CLK_EN_MASK                        BIT(3)
#define RG_DP_RAMP_SEL_MASK                         GENMASK(11, 10)
#define RG_DP_RAMP_SEL_VAL(x)                       (((x) & 0x3) << 10)
#define RG_DEPOP_RAMPGEN_START_MASK                 BIT(14)
#define RG_DEPOP_RAMPGEN_EN_MASK                    BIT(15)
#define RG_DEPOP_RAMPGEN_CAP_RSTB_MASK              BIT(17)

/* AUD_DAC_CON04 */
#define RG_REV_ADAC1_MASK                           BIT(0)
#define RG_DP_PL_EN_MASK                            BIT(30)
#define RG_DP_PL_SEL_MASK                           BIT(29)
#define RG_LDO_LAT_EN_MASK                          BIT(28)
#define RG_LDO_LAT_IQSEL_MASK                       GENMASK(27, 26)
#define RG_LDO_LAT_IQSEL_VAL(x)                     (((x) & 0x3) << 26)
#define RG_LDO_LAT_VOSEL_MASK                       GENMASK(25, 24)
#define RG_LDO_LAT_VOSEL_VAL(x)                     (((x) & 0x3) << 24)

/* AUD_ADC_COMMON */
#define RG_ADC_LDO_VOSEL_MASK                       GENMASK(7, 4)
#define RG_ADC_LDO_VOSEL_VAL(x)                     (((x) & 0xf) << 4)
#define RG_ADC_AUDMICBIASVREF_MASK                  GENMASK(17, 16)
#define RG_ADC_AUDMICBIASVREF_VAL(x)                (((x) & 0x3) << 16)
#define RG_ADC_CLK_SEL_MASK                         BIT(24)
#define RG_ADC_GLBIAS_EN_AUDUL_MASK                 BIT(25)
#define RG_ADC_AUDPWDBMICBIAS_MASK                  BIT(26)
#define RG_ADC_CLK_EN_MASK                          BIT(28)

/* AUD_ADC_RSVD */
#define RG_ADC_AUDUL_REV_MASK                       BIT(0)

/* AUD_ADC_LCH_CON00 */
#define RG_ADC_AUDULL_INT_CHP_EN_MASK               BIT(3)
#define RG_ADC_AUDULL_DITHER_RESETB_MASK            BIT(4)
#define RG_ADC_AUDULL_DITHER_EN_MASK                BIT(5)
#define RG_ADC_AUDULL_RC_TRIM_MASK                  GENMASK(13, 8)
#define RG_ADC_AUDULL_RC_TRIM_VAL(x)                (((x) & 0x3f) << 8)
#define RG_ADC_LDO08_VOSEL_AUDULL_MASK              GENMASK(17, 16)
#define RG_ADC_LDO08_VOSEL_AUDULL_VAL(x)            (((x) & 0x3) << 16)
#define RG_ADC_AUDULL_SARADC_RSTB_MASK              BIT(20)
#define RG_ADC_AUDULL_SARADC_EN_MASK                BIT(21)
#define RG_ADC_AUDULL_VREF_EN_MASK                  BIT(22)
#define RG_ADC_AUDULL_VREF_VCM_EN_MASK              BIT(23)
#define RG_ADC_AUDULL_PGA_PWDB_MASK                 BIT(24)
#define RG_ADC_AUDULL_PGAOUTPUT_EN_MASK             BIT(25)
#define RG_ADC_AUDULL_PDN_INT1OP_MASK               BIT(26)
#define RG_ADC_AUDULL_INT2_RESETB_MASK              BIT(27)
#define RG_ADC_AUDULL_INT1_RESETB_MASK              BIT(28)
#define RG_ADC_LDO08_PWDB_AUDULL_MASK               BIT(29)
#define RG_ADC_LDO_L_PWDB_MASK                      BIT(30)

/* AUD_ADC_LCH_CON03 */
#define RG_ADC_AUDULL_PGA_SDR_EN_MASK               BIT(0)
#define RG_ADC_AUDULL_PGA_NEGR_EN_MASK              BIT(1)
#define RG_ADC_AUDULL_PGA_GAIN_MASK                 GENMASK(7, 4)
#define RG_ADC_AUDULL_PGA_GAIN_VAL(x)               (((x) & 0xf) << 4)
#define RG_ADC_AUDULL_PGA_CHP_EN_MASK               BIT(12)

/* AUD_ADC_LCH_CON04 */
#define RG_ADC_AUDULL_VREF_VCM_CHP_EN_MASK              BIT(0)
#define RG_ADC_AUDULL_VREF_VCM09_SEL_MASK               GENMASK(3, 1)
#define RG_ADC_AUDULL_VREF_VCM09_SEL_VAL(x)             (((x) & 0x7) << 1)
#define RG_ADC_AUDULL_VREF_VCM08_SEL_MASK               GENMASK(6, 4)
#define RG_ADC_AUDULL_VREF_VCM08_SEL_VAL(x)             (((x) & 0x7) << 4)
#define RG_ADC_AUDULL_VREF_VCM1P4_SEL_MASK              GENMASK(11, 8)
#define RG_ADC_AUDULL_VREF_VCM1P4_SEL_VAL(x)            (((x) & 0xf) << 8)
#define RG_ADC_AUDULL_VREF_OUTPUT_CURRENT_SEL_MASK      GENMASK(13, 12)
#define RG_ADC_AUDULL_VREF_OUTPUT_CURRENT_SEL_VAL(x)    (((x) & 0x3) << 12)
#define RG_ADC_AUDULL_VREF_CURRENT_ADJUST_MASK          GENMASK(19, 17)
#define RG_ADC_AUDULL_VREF_CURRENT_ADJUST_VAL(x)        (((x) & 0x7) << 17)
#define RG_ADC_AUDULL_VREF_CHP_EN_MASK                  BIT(20)

/* AUD_ADC_LCH_CON05 */
#define RG_ADC_AUDULL_SARADC_VREF_ISEL_MASK         GENMASK(2, 0)
#define RG_ADC_AUDULL_SARADC_VREF_ISEL_VAL(x)       (((x) & 0x7) << 0)
#define RG_ADC_AUDULL_SARADC_SEL_MASK               GENMASK(15, 8)
#define RG_ADC_AUDULL_SARADC_SEL_VAL(x)             (((x) & 0xff) << 8)
#define RG_ADC_AUDULL_SARADC_CTRL_SEL_MASK          GENMASK(23, 16)
#define RG_ADC_AUDULL_SARADC_CTRL_SEL_VAL(x)        (((x) & 0xff) << 16)
#define RG_ADC_AUDULL_SARADC_SA_DLY_SEL_MASK        GENMASK(25, 24)
#define RG_ADC_AUDULL_SARADC_SA_DLY_SEL_VAL(x)      (((x) & 0x3) << 24)
#define RG_ADC_AUDULL_SARADC_DEC_DLY_SEL_MASK       GENMASK(29, 28)
#define RG_ADC_AUDULL_SARADC_DEC_DLY_SEL_VAL(x)     (((x) & 0x3) << 28)

/* AUD_ADC_RCH_CON00 */
#define RG_ADC_AUDULR_INT_CHP_EN_MASK               BIT(3)
#define RG_ADC_AUDULR_DITHER_RESETB_MASK            BIT(4)
#define RG_ADC_AUDULR_DITHER_EN_MASK                BIT(5)
#define RG_ADC_AUDULR_RC_TRIM_MASK                  GENMASK(13, 8)
#define RG_ADC_AUDULR_RC_TRIM_VAL(x)                (((x) & 0x3f) << 8)
#define RG_ADC_LDO08_VOSEL_AUDULR_MASK              GENMASK(17, 16)
#define RG_ADC_LDO08_VOSEL_AUDULR_VAL(x)            (((x) & 0x3) << 16)
#define RG_ADC_AUDULR_SARADC_RSTB_MASK              BIT(20)
#define RG_ADC_AUDULR_SARADC_EN_MASK                BIT(21)
#define RG_ADC_AUDULR_VREF_EN_MASK                  BIT(22)
#define RG_ADC_AUDULR_VREF_VCM_EN_MASK              BIT(23)
#define RG_ADC_AUDULR_PGA_PWDB_MASK                 BIT(24)
#define RG_ADC_AUDULR_PGAOUTPUT_EN_MASK             BIT(25)
#define RG_ADC_AUDULR_PDN_INT1OP_MASK               BIT(26)
#define RG_ADC_AUDULR_INT2_RESETB_MASK              BIT(27)
#define RG_ADC_AUDULR_INT1_RESETB_MASK              BIT(28)
#define RG_ADC_LDO08_PWDB_AUDULR_MASK               BIT(29)
#define RG_ADC_LDO_R_PWDB_MASK                      BIT(30)

/* AUD_ADC_RCH_CON03 */
#define RG_ADC_AUDULR_PGA_SDR_EN_MASK               BIT(0)
#define RG_ADC_AUDULR_PGA_NEGR_EN_MASK              BIT(1)
#define RG_ADC_AUDULR_PGA_GAIN_MASK                 GENMASK(7, 4)
#define RG_ADC_AUDULR_PGA_GAIN_VAL(x)               (((x) & 0xf) << 4)
#define RG_ADC_AUDULR_PGA_CHP_EN_MASK               BIT(12)

/* AUD_ADC_RCH_CON04 */
#define RG_ADC_AUDULR_VREF_VCM_CHP_EN_MASK              BIT(0)
#define RG_ADC_AUDULR_VREF_VCM09_SEL_MASK               GENMASK(3, 1)
#define RG_ADC_AUDULR_VREF_VCM09_SEL_VAL(x)             (((x) & 0x7) << 1)
#define RG_ADC_AUDULR_VREF_VCM08_SEL_MASK               GENMASK(6, 4)
#define RG_ADC_AUDULR_VREF_VCM08_SEL_VAL(x)             (((x) & 0x7) << 4)
#define RG_ADC_AUDULR_VREF_VCM1P4_SEL_MASK              GENMASK(11, 8)
#define RG_ADC_AUDULR_VREF_VCM1P4_SEL_VAL(x)            (((x) & 0xf) << 8)
#define RG_ADC_AUDULR_VREF_OUTPUT_CURRENT_SEL_MASK      GENMASK(13, 12)
#define RG_ADC_AUDULR_VREF_OUTPUT_CURRENT_SEL_VAL(x)    (((x) & 0x3) << 12)
#define RG_ADC_AUDULR_VREF_CURRENT_ADJUST_MASK          GENMASK(19, 17)
#define RG_ADC_AUDULR_VREF_CURRENT_ADJUST_VAL(x)        (((x) & 0x7) << 17)
#define RG_ADC_AUDULR_VREF_CHP_EN_MASK                  BIT(20)

/* AUD_ADC_RCH_CON05 */
#define RG_ADC_AUDULR_SARADC_VREF_ISEL_MASK         GENMASK(2, 0)
#define RG_ADC_AUDULR_SARADC_VREF_ISEL_VAL(x)       (((x) & 0x7) << 0)
#define RG_ADC_AUDULR_SARADC_SEL_MASK               GENMASK(15, 8)
#define RG_ADC_AUDULR_SARADC_SEL_VAL(x)             (((x) & 0xff) << 8)
#define RG_ADC_AUDULR_SARADC_CTRL_SEL_MASK          GENMASK(23, 16)
#define RG_ADC_AUDULR_SARADC_CTRL_SEL_VAL(x)        (((x) & 0xff) << 16)
#define RG_ADC_AUDULR_SARADC_SA_DLY_SEL_MASK        GENMASK(25, 24)
#define RG_ADC_AUDULR_SARADC_SA_DLY_SEL_VAL(x)      (((x) & 0x3) << 24)
#define RG_ADC_AUDULR_SARADC_DEC_DLY_SEL_MASK       GENMASK(29, 28)
#define RG_ADC_AUDULR_SARADC_DEC_DLY_SEL_VAL(x)     (((x) & 0x3) << 28)

/* AFE_ADDA_DL_SRC2_CON0 */
#define AFE_ADDA_DL_SRC2_CON0_DL_INPUT_MODE_MASK    GENMASK(31, 28)
#define AFE_ADDA_DL_SRC2_CON0_DL_INPUT_MODE_VAL(x)  (((x) & 0xf) << 28)
#define AFE_ADDA_DL_SRC2_CON0_DL_OUTPUT_SEL_MASK    GENMASK(25, 24)
#define AFE_ADDA_DL_SRC2_CON0_DL_OUTPUT_SEL_VAL(x)  (((x) & 0x3) << 24)
#define AFE_ADDA_DL_SRC2_CON0_MUTE_OFF_CTRL_MASK    GENMASK(12, 11)
#define AFE_ADDA_DL_SRC2_CON0_MUTE_OFF              GENMASK(12, 11)
#define AFE_ADDA_DL_SRC2_CON0_DL_VOICE_MODE_MASK    BIT(5)
#define AFE_ADDA_DL_SRC2_CON0_DL_VOICE_MODE(x)      (((x) & 0x1) << 5)
#define AFE_ADDA_DL_SRC2_CON0_GAIN_ON_MASK          BIT(1)
#define AFE_ADDA_DL_SRC2_CON0_GAIN_ON               BIT(1)
#define AFE_ADDA_DL_SRC2_CON0_DL_SRC_ON_MASK        BIT(0)
#define AFE_ADDA_DL_SRC2_CON0_DL_SRC_ON             BIT(0)

/* AFE_ADDA_DL_SRC2_CON1 */
#define AFE_ADDA_DL_SRC2_CON1_GAIN_CTRL_MASK        GENMASK(31, 16)
#define AFE_ADDA_DL_SRC2_CON1_GAIN_CTRL_VAL(x)      (((x) & 0xffff) << 16)
#define AFE_ADDA_DL_SRC2_CON1_GAIN_MODE_MASK        BIT(0)

/* AFE_ADDA_TOP_CON0 */
#define AFE_ADDA_TOP_CON0_LOOPBACK_MODE_MASK        GENMASK(15, 12)
#define AFE_ADDA_TOP_CON0_LOOPBACK_MODE_VAL(x)      (((x) & 0xf) << 12)

/* AFE_ADDA_UL_DL_CON0 */
#define AFE_ADDA_UL_DL_CON0_DL_SW_RESET_MASK            BIT(15)
#define AFE_ADDA_UL_DL_CON0_DL_LOOPBACK_MASK            BIT(3)
#define AFE_ADDA_UL_DL_CON0_DL_LOOPBACK_ON              BIT(3)
#define AFE_ADDA_UL_DL_CON0_DL_INTERCONN_BYPASS_MASK    BIT(2)
#define AFE_ADDA_UL_DL_CON0_DL_INTERCONN_BYPASS         BIT(2)
#define AFE_ADDA_UL_DL_CON0_ADDA_INTF_ON_MASK           BIT(0)
#define AFE_ADDA_UL_DL_CON0_ADDA_INTF_ON                BIT(0)

/* AFE_ADDA_PREDIS_CON0 */
#define AFE_ADDA_PREDIS_CON0_PREDIS_CH1_ON_MASK     BIT(31)
#define AFE_ADDA_PREDIS_CON0_PREDIS_CH1_ON          BIT(31)
#define AFE_ADDA_PREDIS_CON0_PREDIS_A2_CH1_MASK     GENMASK(27, 16)
#define AFE_ADDA_PREDIS_CON0_PREDIS_A2_CH1_VAL(x)   (((x) & 0xfff) << 16)
#define AFE_ADDA_PREDIS_CON0_PREDIS_A3_CH1_MASK     GENMASK(11, 0)
#define AFE_ADDA_PREDIS_CON0_PREDIS_A3_CH1_VAL(x)   ((x) & 0xfff)

/* AFE_ADDA_PREDIS_CON1 */
#define AFE_ADDA_PREDIS_CON1_PREDIS_CH2_ON_MASK     BIT(31)
#define AFE_ADDA_PREDIS_CON1_PREDIS_CH2_ON          BIT(31)
#define AFE_ADDA_PREDIS_CON1_PREDIS_A2_CH2_MASK     GENMASK(27, 16)
#define AFE_ADDA_PREDIS_CON1_PREDIS_A2_CH2_VAL(x)   (((x) & 0xfff) << 16)
#define AFE_ADDA_PREDIS_CON1_PREDIS_A3_CH2_MASK     GENMASK(11, 0)
#define AFE_ADDA_PREDIS_CON1_PREDIS_A3_CH2_VAL(x)   ((x) & 0xfff)

/* AFE_NLE_CFG */
#define AFE_NLE_CFG_SW_RSTB_MASK            BIT(31)
#define AFE_NLE_CFG_SW_RSTB_VAL(x)          (((x) & 0x1) << 31)
#define AFE_NLE_CFG_AFE_NLE_ON_MASK         BIT(0)
#define AFE_NLE_CFG_AFE_NLE_ON              BIT(0)

/* AFE_NLE_PRE_BUF_CFG */
#define AFE_NLE_PRE_BUF_CFG_POINT_END_MASK      GENMASK(10, 0)
#define AFE_NLE_PRE_BUF_CFG_POINT_END_VAL       ((x) & 0x3ff)

/* AFE_NLE_PWR_DET_LCH_CFG
 * AFE_NLE_PWR_DET_RCH_CFG
 */
#define H2L_HOLD_TIME_MS                                (26)
#define AFE_NLE_PWR_DET_CH_CFG_H2L_HOLD_TIME_MASK       GENMASK(28, 24)
#define AFE_NLE_PWR_DET_CH_CFG_H2L_HOLD_TIME_VAL(x)     (((x) & 0x1f) << 24)
#define AFE_NLE_PWR_DET_CH_CFG_H2L_HOLD_TIME_DEF_VAL    (H2L_HOLD_TIME_MS << 24)
#define AFE_NLE_PWR_DET_CH_CFG_NLE_VTH_MASK             GENMASK(23, 0)
#define AFE_NLE_PWR_DET_CH_CFG_NLE_VTH_VAL(x)           ((x) & 0xffffff)

/* AFE_NLE_ZCD_LCH_CFG
 * AFE_NLE_ZCD_RCH_CFG
 */
#define AFE_NLE_ZCD_CH_CFG_ZCD_CHECK_MODE_MASK      BIT(2)
#define AFE_NLE_ZCD_CH_CFG_ZCD_CHECK_MODE_VAL(x)    (x << 2)
#define AFE_NLE_ZCD_CH_CFG_ZCD_MODE_SEL_MASK        GENMASK(1, 0)
#define AFE_NLE_ZCD_CH_CFG_ZCD_MODE_SEL_VAL(x)      ((x) & 0x3)

/* AFE_NLE_GAIN_ADJ_LCH_CFG0
 * AFE_NLE_GAIN_ADJ_RCH_CFG0
 */
#define AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_ADJ_BYPASS_ZCD_MASK    BIT(31)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_ADJ_BYPASS_ZCD_VAL(x)  (x << 31)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_TIME_OUT_MASK               GENMASK(26, 24)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_TIME_OUT_VAL(x)             (((x) & 0x7) << 24)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_HOLD_TIME_PER_JUMP_MASK    GENMASK(22, 20)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_HOLD_TIME_PER_JUMP_VAL(x)  (((x) & 0x7) << 20)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_JUMP_MASK    GENMASK(17, 16)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_JUMP_VAL(x)  (((x) & 0x3) << 16)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_ZCD_MASK     GENMASK(13, 8)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_GAIN_STEP_PER_ZCD_VAL(x)   (x << 8)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MIN_MASK                GENMASK(7, 4)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MIN_VAL(x)              (((x) & 0xf) << 4)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MAX_MASK                GENMASK(2, 0)
#define AFE_NLE_GAIN_ADJ_CH_CFG0_AG_MAX_VAL(x)              ((x) & 0x7)

/* AFE_NLE_GAIN_IMP_LCH_CFG0
 * AFE_NLE_GAIN_IMP_RCH_CFG0
 */
#define AFE_NLE_DIGITAL_GAIN_FIX_MANUAL_MODE        BIT(29)
#define AFE_NLE_DIGITAL_GAIN_FIX_MANUAL_VAL(x)      ((x) << 29)
#define AFE_NLE_ANALOG_GAIN_FIX_MANUAL_MODE         BIT(28)
#define AFE_NLE_ANALOG_GAIN_FIX_MANUAL_VAL(x)       ((x) << 28)
#define AFE_NLE_GAIN_IMP_CH_CFG0_AG_DELAY_MASK      GENMASK(21, 16)
#define AFE_NLE_GAIN_IMP_CH_CFG0_AG_DELAY_VAL(x)    (((x) & 0x3f) << 16)
#define AFE_NLE_DIGITAL_GAIN_MANUAL_VAL_MASK        GENMASK(13, 8)
#define AFE_NLE_DIGITAL_GAIN_MANUAL_VAL(x)          (((x) & 0x3f) << 8)
#define AFE_NLE_ANALOG_GAIN_MANUAL_VAL_MASK         GENMASK(5, 0)
#define AFE_NLE_ANALOG_GAIN_MANUAL_VAL(x)           (((x) & 0x3f))

/* ABB_AFE_CON0 */
#define ABB_AFE_CON0_DL_EN_MASK             BIT(0)
#define ABB_AFE_CON0_DL_EN                  BIT(0)

/* ABB_AFE_CON1 */
#define ABB_AFE_CON1_DL_RATE_MASK           GENMASK(3, 0)
#define ABB_AFE_CON1_DL_RATE(x)             ((x) & 0xf)

/* ABB_AFE_CON5 */
#define ABB_AFE_CON5_SDM_GAIN_VAL_MASK          GENMASK(5, 0)
#define ABB_AFE_CON5_SDM_GAIN_VAL(x)            ((x) & 0x3f)

/* ABB_AFE_CON11 */
#define ABB_AFE_CON11_DC_CTRL                   BIT(9)
#define ABB_AFE_CON11_TOP_CTRL                  BIT(8)
#define ABB_AFE_CON11_DC_CTRL_STATUS            BIT(1)
#define ABB_AFE_CON11_TOP_CTRL_STATUS           BIT(0)

/* AFE_AD_UL_DL_CON0 */
#define AFE_AD_UL_DL_CON0_ADDA_AFE_ON           BIT(0)

/* ABB_ULAFE_CON0 */
#define ABB_ULAFE_CON0_AMIC_CH1_PHASE_SEL_MASK      GENMASK(29, 27)
#define ABB_ULAFE_CON0_AMIC_CH1_PHASE_SEL(x)        (((x) & 0x7) << 27)
#define ABB_ULAFE_CON0_AMIC_CH2_PHASE_SEL_MASK      GENMASK(26, 24)
#define ABB_ULAFE_CON0_AMIC_CH2_PHASE_SEL(x)        (((x) & 0x7) << 24)
#define ABB_ULAFE_CON0_UL_VOICE_MODE_MASK           GENMASK(19, 17)
#define ABB_ULAFE_CON0_UL_VOICE_MODE(x)             (((x) & 0x7) << 17)
#define ABB_ULAFE_CON0_UL_LP_MODE_MASK              GENMASK(15, 14)
#define ABB_ULAFE_CON0_UL_LP_MODE(x)                (((x) & 0x3) << 14)
#define ABB_ULAFE_CON0_UL_IIR_ON_MASK               BIT(10)
#define ABB_ULAFE_CON0_UL_IIR_ON                    BIT(10)
#define ABB_ULAFE_CON0_UL_IIR_MODE_MASK             GENMASK(9, 7)
#define ABB_ULAFE_CON0_UL_IIR_MODE(x)               (((x) & 0x7) << 7)
#define ABB_ULAFE_CON0_UL_FIFO_ON_MASK              BIT(3)
#define ABB_ULAFE_CON0_UL_FIFO_ON                   BIT(3)
/*1:No loopback,0:DAC loopback to ADC need fix 1, loopback can not use in 7933*/
#define ABB_ULAFE_CON0_AD_DA_LOOPBACK_DIS_MASK      BIT(1)
#define ABB_ULAFE_CON0_AD_DA_LOOPBACK_DIS           BIT(1)
#define ABB_ULAFE_CON0_UL_SRC_ON_MASK               BIT(0)
#define ABB_ULAFE_CON0_UL_SRC_ON                    BIT(0)


/* ABB_ULAFE_CON1 */
#define ABB_ULAFE_CON1_UL_GAIN_BYPASS_MASK              BIT(28)
#define ABB_ULAFE_CON1_UL_GAIN_NO_BYPASS                BIT(28)
#define ABB_ULAFE_CON1_UL_SINEGEN_OUTPUT_MASK           BIT(27)
#define ABB_ULAFE_CON1_UL_SINEGEN_OUTPUT                BIT(27)
#define ABB_ULAFE_CON1_UL_SINEGEN_MUTE_MASK             BIT(26)
#define ABB_ULAFE_CON1_UL_SINEGEN_MUTE                  BIT(26)
#define ABB_ULAFE_CON1_UL_SINEGEN_AMPDIV_CH2_MASK       GENMASK(23, 21)
#define ABB_ULAFE_CON1_UL_SINEGEN_AMPDIV_CH2_VAL(x)     (((x) & 0x7) << 21)
#define ABB_ULAFE_CON1_UL_SINEGEN_FREQDIV_CH2_MASK      GENMASK(20, 16)
#define ABB_ULAFE_CON1_UL_SINEGEN_FREQDIV_CH2_VAL(x)    (((x) & 0x1f) << 16)
#define ABB_ULAFE_CON1_UL_SINEGEN_AMPDIV_CH1_MASK       GENMASK(11, 9)
#define ABB_ULAFE_CON1_UL_SINEGEN_AMPDIV_CH1_VAL(x)     (((x) & 0x7) << 9)
#define ABB_ULAFE_CON1_UL_SINEGEN_FREQDIV_CH1_MASK      GENMASK(8, 4)
#define ABB_ULAFE_CON1_UL_SINEGEN_FREQDIV_CH1_VAL(x)    (((x) & 0x1f) << 4)

/* AMIC_GAIN_CON0 */
#define AMIC_GAIN_CON0_SAMPLE_PER_STEP_MASK     GENMASK(15, 8)
#define AMIC_GAIN_CON0_SAMPLE_PER_STEP_VAL(x)   (((x) & 0xff) << 8)
#define AMIC_GAIN_CON0_GAIN_EN_MASK             BIT(0)
#define AMIC_GAIN_CON0_GAIN_ON                  BIT(0)

/* AMIC_GAIN_CON1 */
#define AMIC_GAIN_CON1_TARGET_MASK              GENMASK(19, 0)
#define AMIC_GAIN_CON1_TARGET_VAL(x)            ((x) & 0xfffff)

/* AMIC_GAIN_CON2 */
#define AMIC_GAIN_CON2_DOWN_STEP_MASK           GENMASK(19, 0)
#define AMIC_GAIN_CON2_DOWN_STEP_VAL(x)         ((x) & 0xfffff)

/* AMIC_GAIN_CON3 */
#define AMIC_GAIN_CON3_UP_STEP_MASK             GENMASK(19, 0)
#define AMIC_GAIN_CON3_UP_STEP_VAL(x)           ((x) & 0xfffff)

/* AMIC_GAIN_CUR */
#define AMIC_GAIN_CUR_CUR_GAIN_MASK             GENMASK(19, 0)
#define AMIC_GAIN_CUR_CURT_GAIN_VAL(x)          (((x) & 0xfffff) << 8)

#endif /* #ifndef __MT7933_CODEC_H__ */
