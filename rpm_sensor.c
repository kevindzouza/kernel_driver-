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

#define DEV_NAME "RPM_sensor"

#define SET_TIME_INTERVAL _IOW('k',1,int)
#define GET_TIME_INTERVAL _IOR('k',2,int)

static int sensor_val ;
static atomic_t check = ATOMIC_INIT(0);
static unsigned int interval=500;
static unsigned k;
static struct timer_list timer_1;
static DECLARE_WAIT_QUEUE_HEAD(queue_1);
static DEFINE_MUTEX(mutex_1);


static void timer_callbck_fn(struct timer_list *t)
{
    sensor_val = jiffies%1000;
    atomic_set(&check, 1);
    wake_up_interruptible(&queue_1);

    mod_timer(&timer_1, jiffies + msecs_to_jiffies(interval));

}

static int open_mod(struct inode *inode, struct file *file)
{
    mutex_lock(&mutex_1);
    k++;
    mutex_unlock(&mutex_1);
    pr_info("RPM_SENSOR module opened");
    return 0;
}

static int rel_mod(struct inode *inode, struct file *file)
{
    mutex_lock(&mutex_1);
    k--;
    mutex_unlock(&mutex_1);
    pr_info("RPM_SENSOR module released");
    return 0;
}

static ssize_t read_mod(struct file *file, char __user *buf,
			     size_t len, loff_t *off)
{
    char kbuf[16];
    int size;

    if(wait_event_interruptible(queue_1, atomic_read(&check)))
    {
        return -ERESTARTSYS;

    }

    mutex_lock(&mutex_1);
    size = snprintf(kbuf, sizeof(kbuf), "%d\n",sensor_val);
    atomic_set(&check,0);
    mutex_unlock(&mutex_1);

    if( len < size)
    size = len;

    if(copy_to_user(buf,kbuf,size))
    {
        return -EFAULT;
    }

    return size;
}

static long ioctl_mod(struct file *file, unsigned int cmd, unsigned long arg)
{
    int val;
    
    switch(cmd) {

        case SET_TIME_INTERVAL:
           if (copy_from_user(&val, (int __user *)arg, sizeof(val)))
			return -EFAULT;

           mutex_lock(&mutex_1);
           interval = val;
           mutex_unlock(&mutex_1);
           pr_info("Intervaaal set to %d", val);
           return 0;

            
        
        case GET_TIME_INTERVAL:
           val = interval;
		if (copy_to_user((int __user *)arg, &val, sizeof(val)))
			return -EFAULT;
		return 0;

	default:
		return -ENOTTY;
    }
}
static struct file_operations RPM_ops = {

    .owner = THIS_MODULE,
    .open = open_mod,
    .release = rel_mod,
    .read = read_mod,
    .unlocked_ioctl = ioctl_mod
};

static struct miscdevice rmp_det = {

    .minor = MISC_DYNAMIC_MINOR,
    .name = DEV_NAME,
    .fops = &RPM_ops,
    .mode = 0666
};

static int __init sensor_module_init(void)
{
    int reg = misc_register(&rmp_det);

    if(reg) 
    {
        pr_err("Failed to register device\n");
        return reg;
    }

    pr_info("IOW hex val %x\n" , SET_TIME_INTERVAL);
    pr_info("IOR hex val %x\n" , GET_TIME_INTERVAL); 

    timer_setup(&timer_1, timer_callbck_fn, 0);
    mod_timer(&timer_1, jiffies + msecs_to_jiffies(interval));

    pr_info("loaded module");  

    return 0;

}

static void __exit sensor_module_exit(void)
{
    timer_delete_sync(&timer_1);
    misc_deregister(&rmp_det);
    pr_info("unloaded module");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KEV");
MODULE_DESCRIPTION("kernal timer based dummy sensor with ioctl config");

module_init(sensor_module_init);
module_exit(sensor_module_exit);