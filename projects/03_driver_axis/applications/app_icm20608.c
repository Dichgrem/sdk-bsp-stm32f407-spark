#include <board.h>
#include <icm20608.h>
#include <rtdevice.h>
#include <rtthread.h>

#include <math.h>
#include <stdlib.h> /* atof */

#define DBG_TAG "app_icm"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

/* 复用 main.c 里的 LED_R 引脚（PF12） */
#define PIN_LED_R GET_PIN(F, 12)

/* 默认倾斜报警阈值（角度） */
static float g_tilt_threshold_deg = 30.0f;

/* 报警状态：避免刷屏，并配合回差 */
static rt_uint8_t g_tilt_alarm = 0;

/* 小回差，避免在阈值附近抖动反复报警 */
#define TILT_HYSTERESIS_DEG (2.0f)

rt_mutex_t thread_mutex;

/* msh 命令：设置/查看倾斜阈值（单位：度）
 * 用法：
 *   icm_tilt_set        -> 查看当前阈值
 *   icm_tilt_set 25     -> 设置阈值为 25 度（限制在 1~89）
 */
static void icm_tilt_set(int argc, char **argv) {
  if (argc == 1) {
    rt_kprintf("tilt_threshold = %.1f deg\r\n", g_tilt_threshold_deg);
    return;
  }

  if (argc >= 2) {
    float v = (float)atof(argv[1]);
    if (v < 1.0f)
      v = 1.0f;
    if (v > 89.0f)
      v = 89.0f;

    g_tilt_threshold_deg = v;
    rt_kprintf("tilt_threshold set to %.1f deg\r\n", g_tilt_threshold_deg);
  }
}
MSH_CMD_EXPORT(icm_tilt_set,
               set / show tilt threshold(deg).usage : icm_tilt_set[deg]);

/* 传感器线程 */
void app_icm20608_entry(void *argument) {
  icm20608_device_t dev = argument;
  rt_int16_t accel_x, accel_y, accel_z;
  rt_int16_t gyros_x, gyros_y, gyros_z;
  rt_err_t result;

  /* LED 初始化一次即可（开漏输出，PIN_HIGH = 灭） */
  rt_pin_mode(PIN_LED_R, OUTPUT_OD);
  rt_pin_write(PIN_LED_R, PIN_HIGH);

  for (;;) {
    rt_mutex_take(thread_mutex, RT_WAITING_FOREVER);

    /* 读取三轴加速度 */
    result = icm20608_get_accel(dev, &accel_x, &accel_y, &accel_z);
    if (result == RT_EOK) {
      LOG_D("current accelerometer: accel_x%6d, accel_y%6d, accel_z%6d",
            accel_x, accel_y, accel_z);

      /* ========= 倾斜计算 + 阈值报警 =========
       * 用加速度计计算俯仰/横滚角，取两者绝对值的最大值作为“倾斜幅度”
       * roll  = atan( ay / sqrt(ax^2 + az^2) )
       * pitch = atan(-ax / sqrt(ay^2 + az^2) )
       */
      {
        float ax = (float)accel_x;
        float ay = (float)accel_y;
        float az = (float)accel_z;

        /* 防止分母为 0 的极端情况 */
        float denom_roll = sqrtf(ax * ax + az * az);
        float denom_pitch = sqrtf(ay * ay + az * az);
        if (denom_roll < 1e-6f)
          denom_roll = 1e-6f;
        if (denom_pitch < 1e-6f)
          denom_pitch = 1e-6f;

        float roll = atanf(ay / denom_roll) * (180.0f / 3.1415926f);
        float pitch = atanf(-ax / denom_pitch) * (180.0f / 3.1415926f);

        float tilt = fabsf(roll);
        if (fabsf(pitch) > tilt)
          tilt = fabsf(pitch);

        int roll_i = (int)(roll * 100.0f);
        int pitch_i = (int)(pitch * 100.0f);
        int tilt_i = (int)(tilt * 100.0f);
        int th_i = (int)(g_tilt_threshold_deg * 100.0f);

        LOG_D("tilt: roll=%d.%02d deg, pitch=%d.%02d deg, max=%d.%02d deg "
              "(th=%d.%02d)",
              roll_i / 100, abs(roll_i % 100), pitch_i / 100,
              abs(pitch_i % 100), tilt_i / 100, abs(tilt_i % 100), th_i / 100,
              abs(th_i % 100));

        /* 超限报警 + 回差解除 */
        if (!g_tilt_alarm && tilt > g_tilt_threshold_deg) {
          g_tilt_alarm = 1;
          LOG_W("!!! TILT ALARM !!! tilt=%d.%02d deg > %d.%02d deg",
                tilt_i / 100, abs(tilt_i % 100), th_i / 100, abs(th_i % 100));
        } else if (g_tilt_alarm &&
                   tilt < (g_tilt_threshold_deg - TILT_HYSTERESIS_DEG)) {
          g_tilt_alarm = 0;
          int tilt_i = (int)(tilt * 100.0f);
          LOG_I("tilt back to safe: tilt=%d.%02d deg", tilt_i / 100,
                abs(tilt_i % 100));
        }

        /* 报警时红灯快闪；不报警则熄灭 */
        if (g_tilt_alarm) {
          rt_pin_write(PIN_LED_R, PIN_LOW);
          rt_thread_mdelay(50);
          rt_pin_write(PIN_LED_R, PIN_HIGH);
        } else {
          rt_pin_write(PIN_LED_R, PIN_HIGH);
        }
      }
      /* ========= 倾斜计算结束 ========= */
    } else {
      LOG_E("The sensor does not work");
      break;
    }

    /* 读取三轴陀螺仪（保留你原来的打印） */
    result = icm20608_get_gyro(dev, &gyros_x, &gyros_y, &gyros_z);
    if (result == RT_EOK) {
      LOG_D("current gyroscope    : gyros_x%6d, gyros_y%6d, gyros_z%6d",
            gyros_x, gyros_y, gyros_z);
    } else {
      LOG_E("The sensor does not work");
      break;
    }

    rt_mutex_release(thread_mutex);
    rt_thread_mdelay(1000);
  }
}

int app_icm20608_init(void) {
  icm20608_device_t dev = rt_malloc(sizeof(struct icm20608_device));
  const char *i2c_bus_name = "i2c2";
  rt_err_t result;
  rt_thread_t tid;

  dev = icm20608_init(i2c_bus_name);

  /* 对 icm20608 进行零值校准：采样 10 次，求取平均值作为零值 */
  result = icm20608_calib_level(dev, 10);
  if (result == RT_EOK) {
    LOG_D("The sensor calibrates success");
    LOG_D("accel_offset: X%6d  Y%6d  Z%6d", dev->accel_offset.x,
          dev->accel_offset.y, dev->accel_offset.z);
    LOG_D("gyro_offset : X%6d  Y%6d  Z%6d", dev->gyro_offset.x,
          dev->gyro_offset.y, dev->gyro_offset.z);
  } else {
    LOG_E("The sensor calibrates failure");
    icm20608_deinit(dev);
    return 0;
  }

  thread_mutex = rt_mutex_create("thread_mutex", RT_IPC_FLAG_FIFO);

  tid = rt_thread_create("icm20608", app_icm20608_entry, dev, 1024, 22, 10);
  if (tid != RT_NULL)
    rt_thread_startup(tid);

  return 0;
}

void app_icm_start(void) { rt_mutex_release(thread_mutex); }

void app_icm_stop(void) { rt_mutex_take(thread_mutex, RT_WAITING_FOREVER); }

INIT_DEVICE_EXPORT(app_icm20608_init);

MSH_CMD_EXPORT(app_icm_start, icm start);
MSH_CMD_EXPORT(app_icm_stop, icm stop);
