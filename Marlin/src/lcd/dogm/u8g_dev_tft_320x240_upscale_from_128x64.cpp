/**
 * Marlin 3D Printer Firmware
 * Copyright (C) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (C) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*

  u8g_dev_tft_320x240_upscale_from_128x64.cpp

  Universal 8bit Graphics Library

  Copyright (c) 2011, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "../../inc/MarlinConfig.h"

#if HAS_GRAPHICAL_LCD

#define LCD_USE_DMA_FSMC

#include "U8glib.h"
#include "HAL_LCD_com_defines.h"
#include <string.h>

#define WIDTH 128
#define HEIGHT 64
#define PAGE_HEIGHT 8

#define X_MIN 32
#define Y_MIN 32
#define X_MAX (X_MIN + 2 * WIDTH  - 1)
#define Y_MAX (Y_MIN + 2 * HEIGHT - 1)

#define LCD_COLUMN      0x2A   /* Colomn address register */
#define LCD_ROW         0x2B   /* Row address register */
#define LCD_WRITE_RAM   0x2C

static const uint8_t sd_logo[] = {
   B00000000,B00000000,
   B01110011,B11000000,
   B10001001,B00100000,
   B10000001,B00100000,
   B01110001,B00100000,
   B00001001,B00100000,
   B10001001,B00100000,
   B01110011,B11000000,
};

static const uint8_t usb_logo[] = {
   B00000000,B00000000,B00000000,
   B10001001,B11001111,B00000000,
   B10001010,B00101000,B10000000,
   B10001010,B00001000,B10000000,
   B10001001,B11001111,B00000000,
   B10001000,B00101000,B10000000,
   B10001010,B00101000,B10000000,
   B01110001,B11001111,B00000000,
};


// see https://ee-programming-notepad.blogspot.com/2016/10/16-bit-color-generator-picker.html

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_BLUE  0x21DD
#define COLOR_RED   0xF800
#define COLOR_DARK  0x0003 // Some dark color

#ifndef TFT_MARLINUI_COLOR
#define TFT_MARLINUI_COLOR COLOR_WHITE
#endif
#ifndef TFT_MARLINBG_COLOR
#define TFT_MARLINBG_COLOR COLOR_BLACK
#endif
#ifndef TFT_TOPICONS_COLOR
#define TFT_TOPICONS_COLOR COLOR_BLUE
#endif
#ifndef TFT_DISABLED_COLOR
#define TFT_DISABLED_COLOR COLOR_DARK
#endif
#ifndef TFT_BTSLEFT_COLOR
#define TFT_BTSLEFT_COLOR COLOR_BLUE
#endif
#ifndef TFT_BTRIGHT_COLOR
#define TFT_BTRIGHT_COLOR COLOR_RED
#endif

static uint32_t lcd_id = 0;

#define U8G_ESC_DATA(x) (uint8_t)(x >> 8), (uint8_t)(x & 0xFF)

static const uint8_t page_first_sequence[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), U8G_ESC_DATA(X_MIN), U8G_ESC_DATA(X_MAX),
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), U8G_ESC_DATA(Y_MIN), U8G_ESC_DATA(Y_MAX),
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t clear_screen_sequence[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), 0x00, 0x00, 0x01, 0x3F,
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), 0x00, 0x00, 0x00, 0xEF,
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t sd_sequence[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), U8G_ESC_DATA(32), U8G_ESC_DATA(63),
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), U8G_ESC_DATA(8), U8G_ESC_DATA(31),
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t usb_sequence[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), U8G_ESC_DATA(72), U8G_ESC_DATA(119),
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), U8G_ESC_DATA(8), U8G_ESC_DATA(31),
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t separation_line_sequence_left[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), U8G_ESC_DATA(10), U8G_ESC_DATA(159),
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), U8G_ESC_DATA(170), U8G_ESC_DATA(173),
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t separation_line_sequence_right[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), U8G_ESC_DATA(160), U8G_ESC_DATA(309),
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), U8G_ESC_DATA(170), U8G_ESC_DATA(173),
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t button0_sequence[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), U8G_ESC_DATA(20), U8G_ESC_DATA(99),
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), U8G_ESC_DATA(185), U8G_ESC_DATA(224),
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t button1_sequence[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), U8G_ESC_DATA(120), U8G_ESC_DATA(199),
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), U8G_ESC_DATA(185), U8G_ESC_DATA(224),
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t button2_sequence[] = {
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), U8G_ESC_DATA(220), U8G_ESC_DATA(299),
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), U8G_ESC_DATA(185), U8G_ESC_DATA(224),
  U8G_ESC_ADR(0), LCD_WRITE_RAM, U8G_ESC_ADR(1),
  U8G_ESC_END
};

static const uint8_t st7789v_init_sequence[] = { // 0x8552 - ST7789V
  U8G_ESC_ADR(0),
  0x10,
  U8G_ESC_DLY(10),
  0x01,
  U8G_ESC_DLY(100), U8G_ESC_DLY(100),
  0x11,
  U8G_ESC_DLY(120),
  0x36, U8G_ESC_ADR(1), 0xA0,
  U8G_ESC_ADR(0), 0x3A, U8G_ESC_ADR(1), 0x05,
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), 0x00, 0x00, 0x01, 0x3F,
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), 0x00, 0x00, 0x00, 0xEF,
  U8G_ESC_ADR(0), 0xB2, U8G_ESC_ADR(1), 0x0C, 0x0C, 0x00, 0x33, 0x33,
  U8G_ESC_ADR(0), 0xB7, U8G_ESC_ADR(1), 0x35,
  U8G_ESC_ADR(0), 0xBB, U8G_ESC_ADR(1), 0x1F,
  U8G_ESC_ADR(0), 0xC0, U8G_ESC_ADR(1), 0x2C,
  U8G_ESC_ADR(0), 0xC2, U8G_ESC_ADR(1), 0x01, 0xC3,
  U8G_ESC_ADR(0), 0xC4, U8G_ESC_ADR(1), 0x20,
  U8G_ESC_ADR(0), 0xC6, U8G_ESC_ADR(1), 0x0F,
  U8G_ESC_ADR(0), 0xD0, U8G_ESC_ADR(1), 0xA4, 0xA1,
  U8G_ESC_ADR(0), 0xE0, U8G_ESC_ADR(1), 0xD0, 0x08, 0x11, 0x08, 0x0C, 0x15, 0x39, 0x33, 0x50, 0x36, 0x13, 0x14, 0x29, 0x2D,
  U8G_ESC_ADR(0), 0xE1, U8G_ESC_ADR(1), 0xD0, 0x08, 0x10, 0x08, 0x06, 0x06, 0x39, 0x44, 0x51, 0x0B, 0x16, 0x14, 0x2F, 0x31,
  U8G_ESC_ADR(0), 0x29, 0x11, 0x35, U8G_ESC_ADR(1), 0x00,
  U8G_ESC_END
};

static const uint8_t ili9341_init_sequence[] = { // 0x9341 - ILI9341
  U8G_ESC_ADR(0),
  0x10,
  U8G_ESC_DLY(10),
  0x01,
  U8G_ESC_DLY(100), U8G_ESC_DLY(100),
  0x36, U8G_ESC_ADR(1), 0xE8,
  U8G_ESC_ADR(0), 0x3A, U8G_ESC_ADR(1), 0x55,
  U8G_ESC_ADR(0), LCD_COLUMN, U8G_ESC_ADR(1), 0x00, 0x00, 0x01, 0x3F,
  U8G_ESC_ADR(0), LCD_ROW,    U8G_ESC_ADR(1), 0x00, 0x00, 0x00, 0xEF,
  U8G_ESC_ADR(0), 0xC5, U8G_ESC_ADR(1), 0x3E, 0x28,
  U8G_ESC_ADR(0), 0xC7, U8G_ESC_ADR(1), 0x86,
  U8G_ESC_ADR(0), 0xB1, U8G_ESC_ADR(1), 0x00, 0x18,
  U8G_ESC_ADR(0), 0xC0, U8G_ESC_ADR(1), 0x23,
  U8G_ESC_ADR(0), 0xC1, U8G_ESC_ADR(1), 0x10,
  U8G_ESC_ADR(0), 0x29,
  U8G_ESC_ADR(0), 0x11,
  U8G_ESC_DLY(100),
  U8G_ESC_END
};

static const uint8_t button0[] = {
   B01111111,B11111111,B11111111,B11111111,B11111110,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00010000,B00000000,B00000001,
   B10000000,B00000000,B00010000,B00000000,B00000001,
   B10000000,B01000000,B00010000,B00000000,B00000001,
   B10000000,B11100000,B00010000,B00000000,B00000001,
   B10000001,B11110000,B00010000,B00000000,B00000001,
   B10000011,B11111000,B00010000,B00000000,B00000001,
   B10000111,B11111100,B00010000,B11111111,B11100001,
   B10000000,B11100000,B00010000,B11111111,B11100001,
   B10000000,B11100000,B00010000,B00000000,B00000001,
   B10000000,B11100000,B00010000,B00000000,B00000001,
   B10000000,B11100000,B00010000,B00000000,B00000001,
   B10000000,B11100000,B00010000,B00000000,B00000001,
   B10000000,B00000000,B00010000,B00000000,B00000001,
   B10000000,B00000000,B00010000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B01111111,B11111111,B11111111,B11111111,B11111110,
};

static const uint8_t button1[] = {
   B01111111,B11111111,B11111111,B11111111,B11111110,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00010000,B00000000,B00000001,
   B10000000,B00000000,B00010000,B00000110,B00000001,
   B10000000,B11100000,B00010000,B00000110,B00000001,
   B10000000,B11100000,B00010000,B00000110,B00000001,
   B10000000,B11100000,B00010000,B00000110,B00000001,
   B10000000,B11100000,B00010000,B00000110,B00000001,
   B10000000,B11100000,B00010000,B11111111,B11110001,
   B10000111,B11111100,B00010000,B11111111,B11110001,
   B10000011,B11111000,B00010000,B00000110,B00000001,
   B10000001,B11110000,B00010000,B00000110,B00000001,
   B10000000,B11100000,B00010000,B00000110,B00000001,
   B10000000,B01000000,B00010000,B00000110,B00000001,
   B10000000,B00000000,B00010000,B00000110,B00000001,
   B10000000,B00000000,B00010000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B01111111,B11111111,B11111111,B11111111,B11111110,
};

static const uint8_t button2[] = {
   B01111111,B11111111,B11111111,B11111111,B11111110,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000001,B11000000,B00000001,
   B10000000,B00000000,B01000001,B11000000,B00000001,
   B10000000,B00000000,B11000001,B11000000,B00000001,
   B10000000,B00000001,B11111111,B11000000,B00000001,
   B10000000,B00000011,B11111111,B11000000,B00000001,
   B10000000,B00000001,B11111111,B11000000,B00000001,
   B10000000,B00000000,B11000000,B00000000,B00000001,
   B10000000,B00000000,B01000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B10000000,B00000000,B00000000,B00000000,B00000001,
   B01111111,B11111111,B11111111,B11111111,B11111110,
};

void drawImage(const uint8_t *data, u8g_t *u8g, u8g_dev_t *dev,uint16_t length, uint16_t height, uint16_t color) {
  uint16_t i, j, k;
  uint16_t buffer[160];

  for (i = 0; i < height; i++) {
    k = 0;
    for (j = 0; j < length; j++) {
      if (*(data + (i * (length >> 3) + (j >> 3))) & (128 >> (j & 7))) {
        buffer[k++] = color;
        buffer[k++] = color;
      } else {
        buffer[k++] = 0x0000;
        buffer[k++] = 0x0000;
      }
    }
    u8g_WriteSequence(u8g, dev, length << 2, (uint8_t *)buffer);
    u8g_WriteSequence(u8g, dev, length << 2, (uint8_t *)buffer);
  }
}

// used to fill RGB565 (16bits) background
inline void memset2(const void *ptr, uint16_t fill, size_t cnt) {
  uint16_t* wptr = (uint16_t*) ptr;
  for (size_t i = 0; i < cnt; i += 2) {
     *wptr = fill;
     wptr++;
  }
}

#if defined(SDIO_SUPPORT) && defined(CUSTOM_STATUS_SCREEN_IMAGE)
#define DYNAMIC_DEV_ICONS
#include "../ultralcd.h"
#include "../../sd/cardreader.h"
#include "../../gcode/gcode.h"
static bool sd, usb;
#endif

uint8_t u8g_dev_tft_320x240_upscale_from_128x64_fn(u8g_t *u8g, u8g_dev_t *dev, uint8_t msg, void *arg) {
  u8g_pb_t *pb = (u8g_pb_t *)(dev->dev_mem);
  uint16_t buffer[256]; //16 bit RGB 565 pixel line buffer
  uint16_t i;
  switch (msg) {
    case U8G_DEV_MSG_INIT:
      dev->com_fn(u8g, U8G_COM_MSG_INIT, U8G_SPI_CLK_CYCLE_NONE, &lcd_id);
      if (lcd_id == 0x040404) return 0; // No connected display on FSMC
      if (lcd_id == 0xFFFFFF) return 0; // No connected display on SPI

      memset2(buffer, TFT_MARLINBG_COLOR, sizeof(buffer));

      if ((lcd_id & 0xFFFF) == 0x8552)  // ST7789V
        u8g_WriteEscSeqP(u8g, dev, st7789v_init_sequence);
      if ((lcd_id & 0xFFFF) == 0x9341)  // ILI9341
        u8g_WriteEscSeqP(u8g, dev, ili9341_init_sequence);

      u8g_WriteEscSeqP(u8g, dev, clear_screen_sequence);
      for (i = 0; i < 960; i++)
        u8g_WriteSequence(u8g, dev, 160, (uint8_t *)buffer);

      // bottom line and buttons

      for (i = 0; i < 150; i++) buffer[i] = TFT_DISABLED_COLOR;
      u8g_WriteEscSeqP(u8g, dev, separation_line_sequence_left);
      for (i = 0; i < 4; i++)
        u8g_WriteSequence(u8g, dev, 150, (uint8_t *)buffer);

      for (i = 0; i < 150; i++) buffer[i] = TFT_DISABLED_COLOR;
      u8g_WriteEscSeqP(u8g, dev, separation_line_sequence_right);
      for (i = 0; i < 4; i++)
        u8g_WriteSequence(u8g, dev, 150, (uint8_t *)buffer);

      u8g_WriteEscSeqP(u8g, dev, button0_sequence);
      drawImage(button0, u8g, dev, 40, 20, TFT_BTSLEFT_COLOR);

      u8g_WriteEscSeqP(u8g, dev, button1_sequence);
      drawImage(button1, u8g, dev, 40, 20, TFT_BTSLEFT_COLOR);

      u8g_WriteEscSeqP(u8g, dev, button2_sequence);
      drawImage(button2, u8g, dev, 40, 20, TFT_BTRIGHT_COLOR);

      break;

    case U8G_DEV_MSG_STOP:
      break;

    case U8G_DEV_MSG_PAGE_FIRST:
#ifdef DYNAMIC_DEV_ICONS
      // top icons
      sd = card.isDetected();
      usb = usb_serial_connected;
      //if (ui.on_status_screen()) {
        u8g_WriteEscSeqP(u8g, dev, sd_sequence);
        drawImage(sd_logo, u8g, dev, 16, 8, sd ? TFT_TOPICONS_COLOR : TFT_DISABLED_COLOR);

        u8g_WriteEscSeqP(u8g, dev, usb_sequence);
        drawImage(usb_logo, u8g, dev, 24, 8, usb ? TFT_TOPICONS_COLOR : TFT_DISABLED_COLOR);
      //}
#endif
      u8g_WriteEscSeqP(u8g, dev, page_first_sequence);
      break;

    case U8G_DEV_MSG_PAGE_NEXT:
      for (uint16_t y = 0; y < 8; y++) {
        uint32_t k = 0;
        for (i = 0; i < (uint32_t)pb->width; i++) {
          const uint8_t b = *(((uint8_t *)pb->buf) + i);
          const uint16_t c = TEST(b, y) ? TFT_MARLINUI_COLOR : TFT_MARLINBG_COLOR;
          buffer[k++] = c; buffer[k++] = c;
        }
#ifdef LCD_USE_DMA_FSMC
        for (k = 0; k < 2; k++) {
          extern void LCD_IO_WriteSequence(uint16_t *data, uint16_t length);
          LCD_IO_WriteSequence(buffer, 256);
        }
#else
        for (k = 0; k < 2; k++) {
          u8g_WriteSequence(u8g, dev, 128, (uint8_t*)buffer);
          u8g_WriteSequence(u8g, dev, 128, (uint8_t*)&(buffer[64]));
          u8g_WriteSequence(u8g, dev, 128, (uint8_t*)&(buffer[128]));
          u8g_WriteSequence(u8g, dev, 128, (uint8_t*)&(buffer[192]));
        }
#endif
      }
      break;

    case U8G_DEV_MSG_SLEEP_ON:
    case U8G_DEV_MSG_SLEEP_OFF:
      return 1;
  }
  return u8g_dev_pb8v1_base_fn(u8g, dev, msg, arg);
}

U8G_PB_DEV(u8g_dev_tft_320x240_upscale_from_128x64, WIDTH, HEIGHT, PAGE_HEIGHT, u8g_dev_tft_320x240_upscale_from_128x64_fn, U8G_COM_HAL_FSMC_FN);

#endif // HAS_GRAPHICAL_LCD
