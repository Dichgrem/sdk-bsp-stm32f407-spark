#include <ap3216c.h>
#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>

#define DBG_TAG "app_ap3216c_entry"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* LED 引脚 */
#define PIN_LED_R GET_PIN(F, 12) // 红灯
#define PIN_LED_B GET_PIN(F, 11) // 蓝灯（可选）

ap3216c_device_t dev;

void app_ap3216c_entry(void *argument) {
  rt_uint16_t ps_data;
  float brightness;

  rt_pin_mode(PIN_LED_R, OUTPUT_OD);

  for (;;) {
    /* 读取接近与光照数据 */
    ps_data = ap3216c_read_ps_data(dev);

    brightness = ap3216c_read_ambient_light(dev);
    LOG_D("current brightness: %d.%d(lux).", (int)brightness,
          ((int)(brightness * 10) % 10));

    /* 根据光照控制 LED */
    if (brightness < 50.0f || ps_data > 0)
      rt_pin_write(PIN_LED_R, PIN_LOW); // LED 亮
    else
      rt_pin_write(PIN_LED_R, PIN_HIGH); // LED 灭

    rt_thread_mdelay(500);
  }
}

int app_ap3216c_init(void) {
  const char *i2c_bus_name = "i2c2";
  rt_thread_t tid;

  /* 初始化 AP3216C 设备 */
  dev = ap3216c_init(i2c_bus_name);

  /* 创建线程采集数据并控制 LED */
  tid = rt_thread_create("AP3216C", app_ap3216c_entry, NULL, 1024, 20, 1);
  if (tid != RT_NULL)
    rt_thread_startup(tid);

  return 0;
}

/* 注册为自动初始化设备 */
INIT_DEVICE_EXPORT(app_ap3216c_init);

