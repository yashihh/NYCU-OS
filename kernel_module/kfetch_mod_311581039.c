#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h> /* for get_user and put_user */ 
#include <linux/fs.h> /* register */ 
#include <linux/device.h> /* device create */
#include <linux/printk.h> 
#include "kfetch.h"


char pic1[] = "        .-.";      
char pic2[] = "       (.. |";     
char pic3[] = "       <>  |";     
char pic4[] = "      / --- \\";
char pic5[] = "     ( |   | |";
char pic6[] = "   |\\\\_)___/\\)/\\";
char pic7[] = "  <__)------(__/";

/* Global variables are declared as static, so are global within the file. */ 

static int major; /* major number assigned to our device driver */ 

/* Is device open? Used to prevent multiple access to device */ 
//static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
 
static char kfetch_buf[KFETCH_BUF_SIZE + 1]; 

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

    /* fetching the information */

    if (copy_to_user(buffer, kfetch_buf, length)) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    return 0;
    
    /* cleaning up */
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello */ 
static ssize_t kfetch_write(struct file *filp,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{
    pr_info("kfetch_write\n");

    int mask_info;

    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }

    /* setting the information mask */
    return 0;

}

/* Called when a process tries to open the device file, like 
 * "sudo cat /dev/chardev" 
 */ 
static int kfetch_open(struct inode *inode, struct file *filp) 
{
    try_module_get(THIS_MODULE);
    return 0;
}

/* Called when a process closes the device file. */ 
static int kfetch_release(struct inode *inode, struct file *filp)
{
    pr_info("kfetch_release\n");
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