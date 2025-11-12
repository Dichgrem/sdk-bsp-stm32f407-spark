#include "infrared.h"
#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>

/* 按键 */
#define PIN_KEY0 GET_PIN(C, 0)  /* KEY0 -> PC0, 按下低电平 */
#define PIN_KEY1 GET_PIN(C, 1)  /* KEY1 -> PC1 */
#define PIN_KEY2 GET_PIN(C, 4)  /* KEY2 -> PC4 */
#define PIN_WK_UP GET_PIN(C, 5) /* WK_UP -> PC5, 按下高电平 */

/* LED */
#define PIN_LED_B GET_PIN(F, 11) /* PF11 蓝灯 */
#define PIN_LED_R GET_PIN(F, 12) /* PF12 红灯 */

/* ---------- 模拟红外数据结构 ---------- */
struct fake_ir_data {
  rt_uint8_t addr;
  rt_uint8_t key;
  rt_uint8_t repeat;
};

/* ---------- 全局变量与 IPC 对象 ---------- */
static struct fake_ir_data g_ir_data = {0};
static rt_mutex_t ir_mutex = RT_NULL;
static rt_mq_t ir_mq = RT_NULL;

/* ---------- 按键扫描函数 ---------- */
static rt_int16_t key_scan(void) {
  if (rt_pin_read(PIN_KEY0) == PIN_LOW) {
    rt_thread_mdelay(20);
    if (rt_pin_read(PIN_KEY0) == PIN_LOW)
      return PIN_KEY0;
  }
  return -RT_ERROR;
}

/* ---------- 模拟红外接收线程 ---------- */
static void ir_receive_thread(void *parameter) {
  struct fake_ir_data recv_data;

  while (1) {
    rt_thread_mdelay(1000); /* 每 1 秒模拟接收一次 */

    recv_data.addr = 0x00;
    recv_data.key = (rt_tick_get() & 0xFF);
    recv_data.repeat = 0;

    rt_mutex_take(ir_mutex, RT_WAITING_FOREVER);
    g_ir_data = recv_data;
    rt_mutex_release(ir_mutex);

    rt_mq_send(ir_mq, &recv_data, sizeof(recv_data));

    /* 红灯闪烁表示“接收到红外信号” */
    rt_pin_write(PIN_LED_R, PIN_LOW);
    rt_thread_mdelay(100);
    rt_pin_write(PIN_LED_R, PIN_HIGH);

    rt_kprintf("[I/recv] RECEIVE OK: addr:0x%02X key:0x%02X repeat:%d\n",
               recv_data.addr, recv_data.key, recv_data.repeat);
  }
}

/* ---------- 模拟红外发送线程 ---------- */
static void ir_send_thread(void *parameter) {
  struct fake_ir_data send_data;
  rt_int16_t key;

  while (1) {
    key = key_scan();
    if (key == PIN_KEY0) {
      rt_mutex_take(ir_mutex, RT_WAITING_FOREVER);
      send_data = g_ir_data;
      rt_mutex_release(ir_mutex);

      /* 蓝灯闪烁表示“发送” */
      rt_pin_write(PIN_LED_B, PIN_LOW);
      rt_thread_mdelay(200);
      rt_pin_write(PIN_LED_B, PIN_HIGH);

      rt_kprintf("[I/send] SEND OK: addr:0x%02X key:0x%02X repeat:%d\n",
                 send_data.addr, send_data.key, send_data.repeat);

      /* 防止长按重复触发 */
      while (rt_pin_read(PIN_KEY0) == PIN_LOW)
        rt_thread_mdelay(20);
    }
    rt_thread_mdelay(20);
  }
}

/* ---------- 消息队列打印线程 ---------- */
static void ir_log_thread(void *parameter) {
  struct fake_ir_data msg;
  while (1) {
    if (rt_mq_recv(ir_mq, &msg, sizeof(msg), RT_WAITING_FOREVER) == RT_EOK) {
      rt_kprintf("[I/log ] MSG QUEUE: addr:0x%02X key:0x%02X repeat:%d\n",
                 msg.addr, msg.key, msg.repeat);
    }
  }
}

/* ---------- 系统心跳线程 ---------- */
static void heartbeat_thread(void *parameter) {
  rt_pin_mode(PIN_LED_R, PIN_MODE_OUTPUT);
  while (1) {
    rt_pin_write(PIN_LED_R, PIN_LOW);
    rt_thread_mdelay(50);
    rt_pin_write(PIN_LED_R, PIN_HIGH);
    rt_thread_mdelay(950);
  }
}

/* ---------- 主函数入口 ---------- */
int main(void) {
  /* 初始化按键与 LED 引脚 */
  rt_pin_mode(PIN_KEY0, PIN_MODE_INPUT_PULLUP);
  rt_pin_mode(PIN_KEY1, PIN_MODE_INPUT_PULLUP);
  rt_pin_mode(PIN_KEY2, PIN_MODE_INPUT_PULLUP);
  rt_pin_mode(PIN_WK_UP, PIN_MODE_INPUT_PULLDOWN);

  rt_pin_mode(PIN_LED_R, PIN_MODE_OUTPUT);
  rt_pin_mode(PIN_LED_B, PIN_MODE_OUTPUT);
  rt_pin_write(PIN_LED_R, PIN_HIGH);
  rt_pin_write(PIN_LED_B, PIN_HIGH);

  /* 初始化 IPC 对象 */
  ir_mutex = rt_mutex_create("ir_mutex", RT_IPC_FLAG_FIFO);
  ir_mq =
      rt_mq_create("ir_mq", sizeof(struct fake_ir_data), 8, RT_IPC_FLAG_FIFO);

  /* 创建并启动线程 */
  rt_thread_t tid_recv =
      rt_thread_create("ir_recv", ir_receive_thread, RT_NULL, 1024, 10, 10);
  rt_thread_t tid_send =
      rt_thread_create("ir_send", ir_send_thread, RT_NULL, 1024, 9, 10);
  rt_thread_t tid_log =
      rt_thread_create("ir_log", ir_log_thread, RT_NULL, 1024, 11, 10);
  rt_thread_t tid_beat =
      rt_thread_create("beat", heartbeat_thread, RT_NULL, 512, 20, 20);

  if (tid_recv)
    rt_thread_startup(tid_recv);
  if (tid_send)
    rt_thread_startup(tid_send);
  if (tid_log)
    rt_thread_startup(tid_log);
  if (tid_beat)
    rt_thread_startup(tid_beat);

  return 0;
}


