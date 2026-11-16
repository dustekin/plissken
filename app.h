/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "PLISSKEN"
#define APP_SPLASH_RGB RGB(242, 120, 72)
#define APP_BLOCK 256u
#define APP_TOUCH
#define APP_SPLASH
#define APP_BOOT

enum {
    OBJ_SETTINGS, OBJ_PANEL, OBJ_BLE,
    OBJ_SEQ0, OBJ_SEQ1, OBJ_SEQ2, OBJ_SEQ3, OBJ_SEQ4, OBJ_SEQ5, OBJ_SEQ6, OBJ_SEQ7,
    OBJ_SEQ8, OBJ_SEQ9, OBJ_SEQ10, OBJ_SEQ11, OBJ_SEQ12, OBJ_SEQ13, OBJ_SEQ14, OBJ_SEQ15,
    OBJ_NOBJ
};
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0xDE000u, \
    0x97000u, 0x99000u, 0x9B000u, 0x9D000u, 0x9F000u, 0xA1000u, 0xA3000u, 0xA5000u, \
    0xA7000u, 0xA9000u, 0xAB000u, 0xAD000u, 0xAF000u, 0xB1000u, 0xB3000u, 0xB5000u}

#define APP_HOST_EXPECT
void app_init(void);
void app_splash(void);
void app_boot(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
int app_expect(const char *what, int *got);
