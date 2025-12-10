/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-5-10      ShiHao       first version
 */

#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include <drv_lcd.h>
#include <rttlogo.h>

/* 配置 LED 灯引脚 */
#define PIN_LED_B GET_PIN(F, 11) // PF11 :  LED_B        --> LED
#define PIN_LED_R GET_PIN(F, 12) // PF12 :  LED_R        --> LED

int main(void) {
  rt_device_t lcd_device;

  /* 查找并初始化 LCD 设备 */
  lcd_device = rt_device_find("lcd");
  if (lcd_device == RT_NULL) {
    LOG_E("can't find lcd device!");
    return -RT_ERROR;
  }

  /* 初始化设备 */
  rt_device_init(lcd_device);

  lcd_clear(WHITE);

  /* show RT-Thread logo */
  lcd_show_image(0, 0, 240, 69, image_rttlogo);

  /* set the background color and foreground color */
  lcd_set_color(WHITE, BLACK);

  /* show some string on lcd */
  lcd_show_string(10, 69, 16, "Hello, RT-Thread!");
  lcd_show_string(10, 69 + 16, 24, "RT-Thread");
  lcd_show_string(10, 69 + 16 + 24, 32, "RT-Thread");

  /* draw a line on lcd */
  lcd_draw_line(0, 69 + 16 + 24 + 32, 240, 69 + 16 + 24 + 32);

  /* draw a concentric circles */
  lcd_draw_point(120, 194);
  for (int i = 0; i < 46; i += 4) {
    lcd_draw_circle(120, 194, i);
  }
  return 0;
}
