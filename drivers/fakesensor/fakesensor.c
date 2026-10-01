#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/wait.h>

#define SENSOR_INTERVAL_MS 10

static struct hrtimer sensor_timer;
static ktime_t kt_period;
static DECLARE_WAIT_QUEUE_HEAD(sensor_wq);
static int data_ready = 0;
static uint32_t sensor_val = 0;

static enum hrtimer_restart sensor_timer_callback(struct hrtimer *timer)
{
    sensor_val++;
    data_ready = 1;
    wake_up_interruptible(&sensor_wq);
    hrtimer_forward_now(timer, kt_period);
    return HRTIMER_RESTART;
}

static ssize_t sensor_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int ret;
    ret = wait_event_interruptible(sensor_wq, data_ready != 0);
    if (ret)
        return -ERESTARTSYS;

    data_ready = 0;
    if (copy_to_user(buf, &sensor_val, sizeof(sensor_val)))
        return -EFAULT;

    return sizeof(sensor_val);
}

static const struct file_operations fakesensor_fops = {
    .owner   = THIS_MODULE,
    .read    = sensor_read,
};

static struct miscdevice fakesensor_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "fakesensor",
    .fops  = &fakesensor_fops,
};

static int __init fakesensor_init(void)
{
    int ret = misc_register(&fakesensor_device);
    if (ret) return ret;

    kt_period = ktime_set(0, SENSOR_INTERVAL_MS * 1000000L);
    
    /* API chuẩn cho Linux Kernel 6.15+ */
    hrtimer_setup(&sensor_timer, sensor_timer_callback, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hrtimer_start(&sensor_timer, kt_period, HRTIMER_MODE_REL);

    pr_info("fakesensor: module loaded (period: %d ms)\n", SENSOR_INTERVAL_MS);
    return 0;
}

static void __exit fakesensor_exit(void)
{
    hrtimer_cancel(&sensor_timer);
    misc_deregister(&fakesensor_device);
    pr_info("fakesensor: module unloaded\n");
}

module_init(fakesensor_init);
module_exit(fakesensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Do Buu Phuoc");
MODULE_DESCRIPTION("Fake Sensor Driver with hrtimer");
