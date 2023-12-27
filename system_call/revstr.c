#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/kernel.h>
#include <stdio.h>

/*
 * You must copy the __NR_revstr marco from
 * <your-kernel-build-dir>/arch/x86/include/generated/uapi/asam/unistd_64.h
 * In this example, the value of __NR_revstr is 549
 */
#define __NR_revstr 452

int main(int argc, char *argv[]) {  
    int ret1 = syscall(__NR_revstr, 5, "hello");
    assert(ret1 == 0);

    int ret2 = syscall(__NR_revstr, 11, "5Y573M C411");
    assert(ret2 == 0);

    return 0;
}