[Assignment 3: System Information Fetching Kernel Module](https://hackmd.io/@a3020008/r1Txj5ES6)
## Character Device Driver Kernel Module
### Some commands
#### Compile and Load Module
```shell
$ make    # compile

$ sudo insmod <YOUR_MODULE>.ko    # load module

$ sudo rmmod <YOUR_MODULE> || true >/dev/null # unload module

$ sudo ./kfetch    # user process

Usage:
        ./kfetch [options]
Options:
        -a  Show all information
        -c  Show CPU model name
        -m  Show memory information
        -n  Show the number of CPU cores
        -p  Show the number of processes
        -r  Show the kernel release information
        -u  Show how long the system has been running
```
- `|| true` 忽略不存在的錯誤
- `>/dev/null` 將輸出訊息到null，表示不輸出訊息在terminal

#### Check module

- 列出所有loaded module
    ```
    $ lsmod | grep kfetch
    kfetch_mod_311581039    16384  0
    ```
- 列出module metadata
    ```
    $ modinfo kfetch_mod_311581039.ko
    filename:       /home/ubuntu/NYCU-OS/kernel_module/kfetch_mod_311581039.ko
    license:        GPL
    description:    Retrieve the system information by reading from this device
    author:         yashihh <yashihh.11@nycu.edu.tw>
    srcversion:     38B2219AE91C1ADBC128424
    depends:        
    retpoline:      Y
    name:           kfetch_mod_311581039
    vermagic:       5.19.12-os-311581039 SMP preempt mod_unload modversions 
    ```

### Makefile
```
KERNELDIR := /lib/modules/$(shell uname -r)/build
CURRENT_PATH := $(shell pwd)

obj-m += kfetch_mod_311581039.o

all:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean
```
- `C` 表示切換到指定的目錄中
- `M` 表示模塊源碼目錄
- `obj-m`: 將module編成`.ko`檔，以便之後可以進行載入 
>[!NOTE]
>`obj-y`: 將module編進kernel中，對於 `obj-y` 直接將module編進kernel時，編譯的順序很重要，因為這會影響開機時module init的順序，如果順序沒搞好可能就會遇到問題！
### init and exit
```c
#include <linux/kernel.h> /* We are doing kernel work */ 
#include <linux/module.h> /* Specifically, a module */ 
#include <linux/fs.h> /* Register, file operation */ 
#include <linux/uaccess.h> /* for copy_from_user and put_user */ 

#define DEV_NAME "YOUR_DEVICE"

static int major; /* major number assigned to our device driver */ 

static struct file_operations dev_ops = {
    .owner   = THIS_MODULE,
    .read    = dev_read,
    .write   = dev_write,
    .open    = dev_open,
    .release = dev_release,
};

static int __init dev_init(void){
    major = register_chrdev(0, DEV_NAME, &dev_ops); 
    
    if (major < 0){
        pr_alert("Registering char device failed with %d\n", major);
	    return major;
    }
    
    /* Register class */ 
    cls = class_create(THIS_MODULE, DEV_NAME); 

    /* Create device file under /dev 
     * after this step, device file have generated under /dev/
     */ 
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEV_NAME); 

    return 0;
}

static void __exit dev_exit(void){
    /* Unregister the device file */ 
    device_destroy(cls, MKDEV(major, 0)); 
    
    /* Unregister the class */ 
    class_destroy(cls); 
 
    /* Unregister the device */ 
    unregister_chrdev(major, DEV_NAME); 
}

/* metadata */
MODULE_AUTHOR("yashihh <yashihh.11@nycu.edu.tw>");
MODULE_DESCRIPTION("example");
MODULE_LICENSE("GPL");

module_init(dev_init);
module_exit(dev_exit);
```

#### Register a Device
- Adding a driver to your system means registering it with the kernel.
    ```c
    int register_chrdev(unsigned int major, const char *name, struct file_operations *fops);
    ```
    - `major` 設為 0 ， kernel 會自動分配一個還沒用到的 major number
    - `name` 為 device name
    - 還有另一個function可用 `alloc_chrdev_region()`
>[!NOTE]
>that we didn’t pass the **minor number** to register_chrdev . That is because the kernel doesn’t care about the minor number;
- 註冊完後會新增一個 entry in `/proc/devices`
    ```
    $ cat /proc/devices | grep "kfetch"
    239 kfetch
    ```
#### Unregister a Device
- If the device file is opened by a process and then we remove the kernel module, using the file would cause a call to the memory location where the appropriate function (read/write) used to be.
    ```c
    void unregister_chrdev(unsigned int major, const char *name)
    ```
- in /include/linux/module.h, **counter** 用來 track 有多少 processes 正在使用該 module
    - `try_module_get(THIS_MODULE)` : Increment the reference count of current module.
    - `module_put(THIS_MODULE)` : Decrement the reference count of current module.
    - `module_refcount(THIS_MODULE)` : Return the value of reference count of current module.
- 可以使用 `cat /proc/modules` or `sudo lsmod` 查看 **counter** value
>[!NOTE]
> that you do not have to check the counter within `cleanup_module` because the check will be performed for you by the system call `sys_delete_module`, defined in /include/linux/syscalls.h

### file operations
#### Prevent multiple accesses
```c
enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
}; 
/* Is device open? Used to prevent multiple access to device */ 
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
```
#### `open`
```c
#define BUF_SIZE 256

static char mssg[BUF_SIZE]; 

static int dev_open(struct inode *inode, struct file *filp) 
{
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
        return -EBUSY; 
    
    try_module_get(THIS_MODULE);
    return 0;
}
```
- Concurrenct `atomic`
    ```c
    static inline int atomic_cmpxchg(atomic_t * v, int old, int new);
    ```
    - [補充 study Linux Concurrency](https://hackmd.io/@VIRqdo35SvekIiH4p76B7g/Hkhqgrr1Z/https%3A%2F%2Fhackmd.io%2Fs%2FHy9JyrBw?type=book)
    - 將 `old` 與 `v` 所在的storage中的值比較。如果相等，則將 `new` 存到 `v` 所表示的 address unit 中；不相等，則不改變。
    

#### `release`
```c
struct int dev_release(struct inode *inode, struct file *filp)
{
    /* We're now ready for our next caller */ 
    atomic_set(&already_open, CDEV_NOT_USED);
    
    module_put(THIS_MODULE);
    return 0;
}
```

#### `read`
```c
struct ssize_t dev_read(struct file *filp, char *buffer, size_t length, loff_t offset)
{
    int len;
    
    /*
     *  read out
     */
    
    if (copy_to_user(buffer, msg, len)) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    
    /* Read functions are supposed to return the number of bytes actually inserted into the buffer. */
    /* Most read functions return the number of bytes put into the buffer. */ 
    return len;

}
```

#### `write`
```c
struct ssize_t dev_write(struct file *filp, char *buf, size_t len, loff_t offset)
{
    int info;
    
    /* 
     * write in
     */
    
    if (copy_from_user(&info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }
    
    return length;
}
```

## Results
    $ sudo ./kfetch
                       OSAssignment
            .-.        ------------
           (.. |       Kernel:   5.19.12-os-311581039
           <>  |       CPUs:     4 / 4
          / --- \      CPU:      Intel(R) Core(TM) i5-1038NG7 CPU @ 2.00GHz
         ( |   | |     Mem:      1690 MB / 8805 MB
       |\\_)___/\)/\   Uptime:   439 mins
      <__)------(__/   Procs:    232

    $ sudo ./kfetch -m -n
                       OSAssignment
            .-.        ------------
           (.. |       CPUs:     4 / 4
           <>  |       Mem:      1687 MB / 8805 MB
          / --- \      
         ( |   | |     
       |\\_)___/\)/\   
      <__)------(__/   

## others
- `/proc` 原本是用來跟 process 溝通用的 ( 也是它名稱的由來 )，但是久而久之變成取得有關 kernel 資訊的地方了。

## Reference
[[課程筆記]Linux Driver正點原子課程筆記](https://meetonfriday.com/posts/5523c739/)
[kernel programming guide](https://sysprog21.github.io/lkmpg/#character-device-drivers)
[Linux : lkmpg 粗淺筆記 (1)](https://fdgkhdkgh.medium.com/linux-lkmpg-粗淺筆記-1-256867ad6337)
