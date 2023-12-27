#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE2(revstr, int, len, const char __user *, str)
{
	int ptr = 0;
	int err = 0;

	char buf[30];
	char *reversedstr = kmalloc(len + 1, GFP_KERNEL);
	if (!reversedstr) {
		printk(KERN_ERR " kmalloc() failed!");
		return -EFAULT;
	}
	err = copy_from_user(buf, str, len);
	if (err) {
		printk(KERN_ERR "copy_from_user() failed\n");
		return -EFAULT;
	}
	buf[len] = '\0';

	for (int i = len; i > 0; i--) {
		reversedstr[ptr] = buf[i - 1];
		ptr++;
	}
	reversedstr[len] = '\0';

	printk(KERN_INFO "The origin string: %s\n", buf);
	printk(KERN_INFO "The reversed string: %s\n", reversedstr);
	//kfree(buf);
	kfree(reversedstr);
	return 0;
}