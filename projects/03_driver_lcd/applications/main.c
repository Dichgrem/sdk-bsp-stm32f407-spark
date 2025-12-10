#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include "hz_font.h"
#include <drv_lcd.h>
#include <rttlogo.h>

#define PIN_LED_B GET_PIN(F, 11)
#define PIN_LED_R GET_PIN(F, 12)

int main(void) {
  rt_device_t lcd_device;

  lcd_device = rt_device_find("lcd");
  if (lcd_device == RT_NULL) {
    LOG_E("can't find lcd device!");
    return -RT_ERROR;
  }

  if (rt_device_init(lcd_device) != RT_EOK) {
    LOG_E("lcd device init failed!");
    return -RT_ERROR;
  }

  /* 清屏为白色背景 */
  lcd_clear(WHITE);

  /* 显示 RT-Thread logo（上方） */
  lcd_show_image(0, 0, 240, 69, image_rttlogo);

  /* 设置前景/背景颜色：黑字白底 */
  lcd_set_color(WHITE, BLACK);

  /* 在 logo 下方显示学号 */
  {
    int x = 10;
    int y = 69 + 8; /* logo 下留一点间距 */

    lcd_show_string(x, y, 16, "ID: xxxxxxxx");

    y += 16 + 8; /* 学号下面再空一行 */

    lcd_draw_hz16(x + 0 * 16, y, hz_1_16x16);
    lcd_draw_hz16(x + 1 * 16, y, hz_2_16x16);
    lcd_draw_hz16(x + 2 * 16, y, hz_3_16x16);
  }

  LOG_I("LCD show: ID=xxxxxxxx, Name=your_name");

  while (1) {
    rt_thread_mdelay(1000);
  }

  return 0;
}
