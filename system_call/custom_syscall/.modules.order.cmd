cmd_custom_syscall/modules.order := {  :; } | awk '!x[$$0]++' - > custom_syscall/modules.order
