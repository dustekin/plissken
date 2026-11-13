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

enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_BLE, OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0xDE000u}

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
