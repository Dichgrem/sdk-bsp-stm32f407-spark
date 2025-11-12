#include <rtthread.h>

static rt_sem_t sem = RT_NULL;
static int count = 0; /* 全局计数器 */

static void thread1_entry(void *parameter) {
  while (1) {
    rt_sem_take(sem, RT_WAITING_FOREVER);
    count++;
    rt_kprintf("thread1: got semaphore! (count = %d)\n", count);
  }
}

static void thread2_entry(void *parameter) {
  while (1) {
    rt_thread_mdelay(500);
    rt_kprintf("thread2: release semaphore (count = %d)\n", count + 1);
    rt_sem_release(sem);
  }
}

int semaphore_demo_init(void) {
  sem = rt_sem_create("sem", 0, RT_IPC_FLAG_FIFO);
  if (sem == RT_NULL) {
    rt_kprintf("failed to create semaphore!\n");
    return -1;
  }

  rt_thread_t t1 = rt_thread_create("t1", thread1_entry, RT_NULL, 1024, 10, 10);
  rt_thread_t t2 = rt_thread_create("t2", thread2_entry, RT_NULL, 1024, 11, 10);

  if (t1 != RT_NULL)
    rt_thread_startup(t1);
  if (t2 != RT_NULL)
    rt_thread_startup(t2);

  return 0;
}

/* 让这个例子自动运行 */
INIT_APP_EXPORT(semaphore_demo_init);

