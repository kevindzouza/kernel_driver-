// vsensor.c - Virtual sensor character device driver
//
// Demonstrates: misc device registration, kernel timer, wait-queue-based
// blocking I/O, and ioctl-based configuration.


#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/jiffies.h>

#define DEVICE_NAME "vsensor0"
#define VSENSOR_SET_INTERVAL_MS _IOW('k', 1, int)
#define VSENSOR_GET_INTERVAL_MS _IOR('k', 2, int)

static struct timer_list sample_timer;
static DECLARE_WAIT_QUEUE_HEAD(vsensor_wq);
static DEFINE_MUTEX(vsensor_lock);

static int sensor_value;
static atomic_t data_ready = ATOMIC_INIT(0);
static unsigned int interval_ms = 500;
static unsigned int open_count;

static void timer_cb(struct timer_list *t)
{
	sensor_value = (sensor_value + (jiffies % 37)) % 1000;
	atomic_set(&data_ready, 1);
	wake_up_interruptible(&vsensor_wq);

	mod_timer(&sample_timer, jiffies + msecs_to_jiffies(interval_ms));
}

static int vsensor_open(struct inode *inode, struct file *file)
{
	mutex_lock(&vsensor_lock);
	open_count++;
	mutex_unlock(&vsensor_lock);
	pr_info("vsensor: opened (count=%u)\n", open_count);
	return 0;
}

static int vsensor_release(struct inode *inode, struct file *file)
{
	mutex_lock(&vsensor_lock);
	open_count--;
	mutex_unlock(&vsensor_lock);
	pr_info("vsensor: released (count=%u)\n", open_count);
	return 0;
}

static ssize_t vsensor_read(struct file *file, char __user *buf,
			     size_t len, loff_t *off)
{
	char kbuf[16];
	int n;

	if (wait_event_interruptible(vsensor_wq, atomic_read(&data_ready)))
		return -ERESTARTSYS;
	
	mutex_lock(&vsensor_lock);
	n = scnprintf(kbuf, sizeof(kbuf), "%d\n", sensor_value);
	atomic_set(&data_ready, 0);
	mutex_unlock(&vsensor_lock);

	if (len < n)
		n = len;
	if (copy_to_user(buf, kbuf, n))
		return -EFAULT;

	return n;
}

static long vsensor_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int val;

	switch (cmd) {
	case VSENSOR_SET_INTERVAL_MS:
		if (copy_from_user(&val, (int __user *)arg, sizeof(val)))
			return -EFAULT;

		mutex_lock(&vsensor_lock);
		interval_ms = val;
		mutex_unlock(&vsensor_lock);
		pr_info("vsensor: sampling interval set to %d ms\n", val);
		return 0;

	case VSENSOR_GET_INTERVAL_MS:
		val = interval_ms;
		if (copy_to_user((int __user *)arg, &val, sizeof(val)))
			return -EFAULT;
		return 0;

	default:
		return -ENOTTY;
	}
}

static const struct file_operations vsensor_fops = {
	.owner          = THIS_MODULE,
	.open           = vsensor_open,
	.release        = vsensor_release,
	.read           = vsensor_read,
	.unlocked_ioctl = vsensor_ioctl,
};

static struct miscdevice vsensor_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = DEVICE_NAME,
	.fops  = &vsensor_fops,
	.mode  = 0666,
};

static int __init vsensor_init(void)
{
	int ret = misc_register(&vsensor_dev);

	if (ret) {
		pr_err("vsensor: misc_register failed (%d)\n", ret);
		return ret;
	}

	timer_setup(&sample_timer,timer_cb, 0);
	mod_timer(&sample_timer, jiffies + msecs_to_jiffies(interval_ms));

	pr_info("vsensor: loaded, /dev/%s ready\n", DEVICE_NAME);
	return 0;
}

static void __exit vsensor_exit(void)
{
	timer_delete_sync(&sample_timer);
	misc_deregister(&vsensor_dev);
	pr_info("vsensor: unloaded\n");
}

module_init(vsensor_init);
module_exit(vsensor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kevin Dsouza");
MODULE_DESCRIPTION("Virtual sensor char device: timer-driven, blocking read, ioctl config");