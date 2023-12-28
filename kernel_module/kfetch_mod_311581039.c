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

#define INFO_COUNT 6
#define INFO_SIZE 50

char info0[INFO_SIZE];
char info1[INFO_SIZE];
char info2[INFO_SIZE];
char info3[INFO_SIZE];
char info4[INFO_SIZE];
char info5[INFO_SIZE];
char dash[INFO_SIZE];

char *infoPointers[INFO_COUNT] = {info0, info1, info2, info3, info4, info5};

char sys_kernel[INFO_SIZE];
char sys_cpu[INFO_SIZE];
char sys_cpus[INFO_SIZE];
char sys_mem[INFO_SIZE];
char sys_procs[INFO_SIZE];
char sys_uptime[INFO_SIZE];



/* Global variables are declared as static, so are global within the file. */ 

static int major; /* major number assigned to our device driver */ 


enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
}; 
/* Is device open? Used to prevent multiple access to device */ 
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
 
static char kfetch_buf[KFETCH_BUF_SIZE]; 

static struct class *cls;

/* Method*/

/* Called when a process, which already opened the dev file, attempts to 
 * read from it. 
 */ 

bool flag = false;

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

    /* fetching the information */
    char *hostname = init_utsname()->nodename;
    int namelen = 0;
    while (namelen < __NEW_UTS_LEN && hostname[namelen] != '\0') {
        namelen++;
    }
    memset(dash, '-', namelen );
    sprintf(sys_kernel, "Kernel:   %s", init_utsname()->release);
    sprintf(sys_cpu, "CPU:      %s", cpuinfo->x86_model_id );
    sprintf(sys_cpus, "CPUs:     %lu / %lu", num_online_cpus(), num_present_cpus());
    sprintf(sys_mem, "Mem:      %lu MB / %lu MB", i.freeram * 4 / 1024, i.totalram * 4 / 1024 );
    sprintf(sys_procs, "Procs:    %u", runningProcs );
    sprintf(sys_uptime, "Uptime:   %lu mins", uptime / 60);

    if (!flag){
        sprintf(info0, "%s", sys_kernel);
        sprintf(info1, "%s", sys_cpu);
        sprintf(info2, "%s", sys_cpus);
        sprintf(info3, "%s", sys_mem);
        sprintf(info4, "%s", sys_procs);
        sprintf(info5, "%s", sys_uptime);
    }
    len = sprintf(kfetch_buf, 
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n"
                    "%s%s\n",
                    pic0, hostname,
                    pic1, dash, 
                    pic2, info0,
                    pic3, info1,
                    pic4, info2,
                    pic5, info3,
                    pic6, info4,
                    pic7, info5);

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
    int mask_info;
    char **ptr = infoPointers;

    flag = true;

    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }

    pr_info("[kfetch_write] mask_info: [%d]\n", mask_info);

    /* setting the information mask */
    for (int i = 0; i < INFO_COUNT; ++i) {
        memset(infoPointers[i], 0, INFO_SIZE);
    }
    if (mask_info & KFETCH_RELEASE) {
        sprintf(*ptr, "%s",sys_kernel);
        **ptr++;
    }
    if (mask_info & KFETCH_NUM_CPUS) {
        sprintf(*ptr, "%s",sys_cpus);
        **ptr++;
    }
    if (mask_info & KFETCH_CPU_MODEL) {
        sprintf(*ptr, "%s",sys_cpu);
        **ptr++;
    }
    if (mask_info & KFETCH_MEM) {
        sprintf(*ptr, "%s",sys_mem);
        **ptr++;

    }
    if (mask_info & KFETCH_UPTIME) {
        sprintf(*ptr, "%s",sys_uptime);
        **ptr++;

    }
    if (mask_info & KFETCH_NUM_PROCS) {
        sprintf(*ptr, "%s",sys_procs);
    }

    return length;

}

/* Called when a process tries to open the device file, like 
 * "sudo cat /dev/chardev" 
 */ 
static int kfetch_open(struct inode *inode, struct file *filp) 
{
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
        return -EBUSY; 
    
    try_module_get(THIS_MODULE); // Increment the reference count
    return 0;
}

/* Called when a process closes the device file. */ 
static int kfetch_release(struct inode *inode, struct file *filp)
{
    /* We're now ready for our next caller */ 
    atomic_set(&already_open, CDEV_NOT_USED);

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