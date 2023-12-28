KERNELDIR := /lib/modules/$(shell uname -r)/build
CURRENT_PATH := $(shell pwd)

obj-m += kfetch_mod_311581039.o

# 忽略 warning
CFLAGS_kfetch_mod_311581039.o := -w

all:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) modules

clean:
	make -C $(KERNELDIR) M=$(CURRENT_PATH) clean

load:
	sudo insmod kfetch_mod_311581039.ko

unload:
	sudo rmmod kfetch_mod_311581039 || true >/dev/null

# || true 忽略不存在的錯誤
# >/dev/null 將輸出訊息到null，表示不輸出訊息在terminal