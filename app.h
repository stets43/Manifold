/* SPDX-License-Identifier: MPL-2.0 */
/* MANIFOLD: the app's half of the fm1-base contract (device/unity.c, host/plat_host.c). */
#pragma once
#include <stdint.h>
#include "plat.h"

#define APP_NAME "MANIFOLD"
#define APP_SPLASH_RGB RGB(120, 190, 255)
#define APP_BLOCK 256u                   /* frames per audio block: 5.8 ms at 44.1 kHz (four 64-sample control ticks); the I2S half buffer too, so an interrupt held up a millisecond or two by the radio or USB is not a dropout */
#define APP_TOUCH                        /* velocity and pressure from the keys (lib/touch.h) */
#define APP_BLE_AT_BOOT (manifold_bt_on())   /* the radio at power-on: as the player left it (Setup) */
int manifold_bt_on(void);
#define APP_OVERLOAD                     /* the platform tells us when a render ran past 90 % (counted, shown) */
void app_overload(void);

/* the store: settings (with the profiler's cuts), the panel calibration (the platform's), 16 patch slots (each with
 * its four scenes), Bluetooth's */
enum { OBJ_SETTINGS, OBJ_PANEL,
       OBJ_P0, OBJ_P1, OBJ_P2, OBJ_P3, OBJ_P4, OBJ_P5, OBJ_P6, OBJ_P7,
       OBJ_P8, OBJ_P9, OBJ_P10, OBJ_P11, OBJ_P12, OBJ_P13, OBJ_P14, OBJ_P15,
       OBJ_BLE,                          /* Bluetooth: the radio's address and the audio devices' keys */
       OBJ_NOBJ };
/* Bluetooth audio (fm1-base's own stack): the radio's receive queue, between its interrupt and the main loop. A packet
   acknowledged on air and dropped here when the queue is full is lost for good, and the audio set-up stalls; fm1-base
   sets 12 for light firmwares. MANIFOLD's main loop is slower (screen updates under a busy audio render): 48, about
   17 KB of the pool (13 KB more than 12) */
#define BTI_RING 48u
#define N_PATCH 16
#define APP_STORE_MAP {0xFC000u, 0xDC000u, \
    0x97000u, 0x99000u, 0x9B000u, 0x9D000u, 0x9F000u, 0xA1000u, 0xA3000u, 0xA5000u, \
    0xA7000u, 0xA9000u, 0xAB000u, 0xAD000u, 0xAF000u, 0xB1000u, 0xB3000u, 0xB5000u, \
    0xDE000u}

/* the contract: the platform calls these */
void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);

#define APP_HOST_EXPECT
int app_expect(const char *what, int *got);
