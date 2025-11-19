#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#define PIN_KEY1 GET_PIN(C, 1)
#define PIN_WK_UP GET_PIN(C, 5)
#define PIN_BEEP GET_PIN(B, 0)

/* 事件 */
#define EVENT_KEY1 (1 << 0)
#define EVENT_WKUP (1 << 1)

/* 按键事件对象 */
static rt_event_t key_event;

/* 中断回调：只发事件，不做逻辑 */
void irq_callback(void *args) {
  rt_uint32_t pin = (rt_uint32_t)args;

  if (pin == PIN_KEY1)
    rt_event_send(key_event, EVENT_KEY1);

  else if (pin == PIN_WK_UP)
    rt_event_send(key_event, EVENT_WKUP);
}

/* 按键状态机结构体 */
typedef struct {
  rt_uint32_t pin;
  rt_uint8_t stable_state; /* 最终稳定状态 (0 或 1) */
  rt_uint8_t last_state;   /* 上一次读到 */
  rt_uint8_t count;        /* 稳定计数器 */
} key_filter_t;

static key_filter_t key1 = {PIN_KEY1, 1, 1, 0};
static key_filter_t wkup = {PIN_WK_UP, 1, 1, 0};

/* 消抖采样函数 */
int key_filter(key_filter_t *k) {
  int now = rt_pin_read(k->pin);

  if (now == k->last_state) {
    if (++k->count >= 3) // 3*10ms = 30ms 消抖
    {
      k->count = 0;
      if (k->stable_state != now) {
        k->stable_state = now;
        return now; // 状态改变
      }
    }
  } else {
    k->count = 0;
  }

  k->last_state = now;
  return -1; // 状态未变化
}

static void beep_thread_entry(void *parameter) {
  rt_uint32_t recv;

  while (1) {
    /* 每 10ms 扫描一次按键 */
    rt_thread_mdelay(10);

    /* 消抖处理 */
    int s1 = key_filter(&key1);
    int s2 = key_filter(&wkup);

    /* KEY1 按下 (低电平) */
    if (s1 == 0) {
      rt_pin_write(PIN_BEEP, PIN_LOW);
      LOG_I("beep OFF");
    }

    /* WK_UP 按下 */
    if (s2 == 0) {
      rt_pin_write(PIN_BEEP, PIN_HIGH);
      LOG_I("beep ON");
    }
  }
}

int main(void) {
  /* 创建事件 */
  key_event = rt_event_create("key_evt", RT_IPC_FLAG_FIFO);

  /* 配置 IO */
  rt_pin_mode(PIN_KEY1, PIN_MODE_INPUT_PULLUP);
  rt_pin_mode(PIN_WK_UP, PIN_MODE_INPUT_PULLUP);
  rt_pin_mode(PIN_BEEP, PIN_MODE_OUTPUT);

  /* 中断（不做消抖） */
  rt_pin_attach_irq(PIN_KEY1, PIN_IRQ_MODE_FALLING, irq_callback,
                    (void *)PIN_KEY1);
  rt_pin_attach_irq(PIN_WK_UP, PIN_IRQ_MODE_FALLING, irq_callback,
                    (void *)PIN_WK_UP);
  rt_pin_irq_enable(PIN_KEY1, PIN_IRQ_ENABLE);
  rt_pin_irq_enable(PIN_WK_UP, PIN_IRQ_ENABLE);

  /* 创建任务 */
  rt_thread_t thread =
      rt_thread_create("beep", beep_thread_entry, RT_NULL, 1024, 10, 10);
  rt_thread_startup(thread);

  return 0;
}
