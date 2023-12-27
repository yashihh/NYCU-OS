#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE0(hello){
    printk(KERN_INFO "Hello World\n");
    printk(KERN_INFO "311581039\n");
    return 0;
}
