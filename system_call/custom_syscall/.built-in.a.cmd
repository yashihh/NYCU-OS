cmd_custom_syscall/built-in.a := rm -f custom_syscall/built-in.a; echo hello.o revstr.o | sed -E 's:([^ ]+):custom_syscall/\1:g' | xargs ar cDPrST custom_syscall/built-in.a
