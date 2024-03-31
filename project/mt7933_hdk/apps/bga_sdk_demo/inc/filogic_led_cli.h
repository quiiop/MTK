

#ifndef __FILOGIC_LED_H__
#define __FILOGIC_LED_H__


#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "cli.h"

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

/*****************************************************************************
 * Enums
 *****************************************************************************/


typedef enum {
    FILOGIC_LED_OFF,
    FILOGIC_LED_R,
    FILOGIC_LED_G,
    FILOGIC_LED_B,
} filogic_led_color_t;

typedef enum {
    FILOGIC_LED_1 = 1,
    FILOGIC_LED_2,
} filogic_led_sn_t;

/*****************************************************************************
 * Structures
 *****************************************************************************/


/*****************************************************************************
 * Functions
 *****************************************************************************/
extern cmd_t led_cli_cmds[];
#define LED_TEST_CLI_ENTRY      {"led", "LED TEST", NULL, led_cli_cmds},
#define led_print(x...) printf("[LED] " x)

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */


#endif /* #ifndef __FILOGIC_LED_H__ */
