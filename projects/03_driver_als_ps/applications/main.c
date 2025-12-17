#include <ap3216c.h>
#include <rtdevice.h>
#include <rtthread.h>

int main(void) {
  ap3216c_device_t dev;

  /* 初始化 AP3216C */
  dev = ap3216c_init("i2c2");
  if (dev == RT_NULL) {
    rt_kprintf("AP3216C initialization failed! Check I2C wiring.\n");
    return 0;
  }

  rt_kprintf("AP3216C found!\n");
  return 0;
}

