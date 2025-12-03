#include <board.h>
#include <rtdevice.h>
#include <rtthread.h>

// ---------------------- GPIO LED -----------------------
#define LED0_PIN GET_PIN(F, 11)
#define LED1_PIN GET_PIN(F, 12)

// ---------------------- EXTI 按键 -----------------------
#define KEY0_PIN GET_PIN(C, 0)

// 定时器句柄
static rt_timer_t led_timer = RT_NULL;

// 定时器回调：LED0 翻转
static void led_timer_callback(void *parameter) {
  rt_pin_write(LED0_PIN, !rt_pin_read(LED0_PIN));
}

// 外部中断回调：按键触发时点亮 LED1
static void key_irq_callback(void *args) {
  rt_kprintf("KEY0 Pressed!\n");
  rt_pin_write(LED1_PIN, !rt_pin_read(LED1_PIN)); // 翻转 LED1
}

int main(void) {
  // 初始化 LED GPIO
  rt_pin_mode(LED0_PIN, PIN_MODE_OUTPUT);
  rt_pin_mode(LED1_PIN, PIN_MODE_OUTPUT);

  // 初始化按键 EXTI
  rt_pin_mode(KEY0_PIN, PIN_MODE_INPUT_PULLUP);
  rt_pin_attach_irq(KEY0_PIN, PIN_IRQ_MODE_FALLING, key_irq_callback, RT_NULL);
  rt_pin_irq_enable(KEY0_PIN, PIN_IRQ_ENABLE);

  // 创建软件定时器：500ms
  led_timer = rt_timer_create("ledtm", led_timer_callback, RT_NULL,
                              500, // 500ms
                              RT_TIMER_FLAG_PERIODIC);

  rt_timer_start(led_timer);

  rt_kprintf("GPIO + Timer + EXTI demo running...\n");

  while (1) {
    rt_thread_mdelay(1000);
  }
}
