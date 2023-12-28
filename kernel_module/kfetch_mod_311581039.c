#include <linux/module.h> /* module reference counter */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h> /* for get_user and put_user */ 
#include <linux/fs.h> /* register */ 
#include <linux/device.h> /* device create */
#include <linux/printk.h>
#include <linux/mm.h> /* si_meminfo() */
#include <linux/sysinfo.h> /* to get system info. e.g. cpu, mem*/
#include <linux/utsname.h> /* to get system info. e.g. nodename, release */
#include <linux/jiffies.h>
#include "kfetch.h"
#include <linux/ktime.h>

char pic0[] = "                   ";
char pic1[] = "        .-.        ";      
char pic2[] = "       (.. |       ";     
char pic3[] = "       <>  |       ";     
char pic4[] = "      / --- \\      ";
char pic5[] = "     ( |   | |     ";
char pic6[] = "   |\\\\_)___/\\)/\\   ";
char pic7[] = "  <__)------(__/   ";
char dash[] = "-------------";

char info0[] = "\0";
char info1[] = "\0";
char info2[] = "\0";
char info3[] = "\0";
char info4[] = "\0";
char info5[] = "\0";

char sys_kernel[30];
char sys_cpu[50];
char sys_cpus[10];
char sys_mem[20];
char sys_procs[10];
char sys_uptime[10];

/* Global variables are declared as static, so are global within the file. */ 

static int major; /* major number assigned to our device driver */ 

/* Is device open? Used to prevent multiple access to device */ 
//static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
 
static char kfetch_buf[KFETCH_BUF_SIZE]; 

static struct class *cls;

/* Method*/

/* Called when a process, which already opened the dev file, attempts to 
 * read from it. 
 */ 
static ssize_t kfetch_read(struct file *filp,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{
    pr_info("kfetch_read\n");
    int len;
    struct cpuinfo_x86 *cpuinfo = &cpu_data(0);
    struct sysinfo i;
    si_meminfo(&i);
    
    s64  uptime;
    uptime = ktime_to_ms(ktime_get_boottime()) / 1000;
    unsigned int runningProcs = 0;

    struct task_struct *task;
    for_each_process(task) {
        runningProcs++;
    }
    sprintf(sys_kernel, "%s", init_utsname()->release);
    sprintf(sys_cpu, "%s", cpuinfo->x86_model_id );
    sprintf(sys_cpus, "%lu / %lu", num_online_cpus(), num_present_cpus());
    sprintf(sys_mem, "%lu MB / %lu MB", i.freeram * 4 / 1024, i.totalram * 4 / 1024 );
    sprintf(sys_procs, "%u", runningProcs );
    sprintf(sys_uptime, "%lu mins", uptime / 60);
    len = sprintf(kfetch_buf, 
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n",
                    pic0, init_utsname()->nodename,
                    pic1, dash, 
                    pic2, sys_kernel,
                    pic3, sys_cpu,
                    pic4, sys_cpus,
                    pic5, sys_mem,
                    pic6, sys_procs,
                    pic7, sys_uptime);
    /* fetching the information */


    if (copy_to_user(buffer, kfetch_buf, len)) {
        pr_alert("Failed to copy data to user");
        return 0;
    }

    /* cleaning up */
    memset(kfetch_buf, 0, KFETCH_BUF_SIZE);

    /* Read functions are supposed to return the number of bytes actually 
     * inserted into the buffer. 
     */ 
    return len;
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello */ 
static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    //memcpy(array2, array1, sizeof(array2));
    int mask_info;

    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }

    /* setting the information mask */
    if (mask_info & KFETCH_RELEASE) {

    }
    if (mask_info & KFETCH_NUM_CPUS) {

    }
    if (mask_info & KFETCH_CPU_MODEL) {

    }
    if (mask_info & KFETCH_MEM) {

    }
    if (mask_info & KFETCH_UPTIME) {

    }
    if (mask_info & KFETCH_NUM_PROCS) {

    }

    pr_info("[kfetch_write] mask_info: [%d]\n", mask_info);
    return 0;

}

/* Called when a process tries to open the device file, like 
 * "sudo cat /dev/chardev" 
 */ 
static int kfetch_open(struct inode *inode, struct file *filp) 
{
    try_module_get(THIS_MODULE); // Increment the reference count
    return 0;
}

/* Called when a process closes the device file. */ 
static int kfetch_release(struct inode *inode, struct file *filp)
{
    module_put(THIS_MODULE); // Decrement the reference count
    return 0;
}

static struct file_operations kfetch_ops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release,
};

static int __init kfetch_init(void)
{
    /* Major set to 0, kernel 會自動分配一個還沒用到的 major number */ 
    major = register_chrdev(0, KFETCH_DEV_NAME, &kfetch_ops); 
    
    if (major < 0){
        pr_alert("Registering char device failed with %d\n", major);
	    return major;

    }
    
    /* Register class */ 
    cls = class_create(THIS_MODULE, KFETCH_DEV_NAME); 

    /* Create device file under /dev 
     * after this step, device file have generated under /dev/
     */ 
    device_create(cls, NULL, MKDEV(major, 0), NULL, KFETCH_DEV_NAME); 

    pr_info("kfetch module init\n");
	return 0;
}

static void __exit kfetch_exit(void)
{
    /* Unregister the device file */ 
    device_destroy(cls, MKDEV(major, 0)); 
    
    /* Unregister the class */ 
    class_destroy(cls); 
 
    /* Unregister the device */ 
    unregister_chrdev(major, KFETCH_DEV_NAME); 

    pr_info("kfetch module exit\n");
}



MODULE_AUTHOR("yashihh <yashihh.11@nycu.edu.tw>");
MODULE_DESCRIPTION("Retrieve the system information by reading from this device");
MODULE_LICENSE("GPL");

module_init(kfetch_init);
module_exit(kfetch_exit);