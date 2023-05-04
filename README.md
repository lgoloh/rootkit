# rootkit

A rootkit: a kernel module designed to intercept system calls to hide the subversive behaviors of another program.

Components:
- sneaky_process.c: A program that modifies the /etc/passwd file and loads the sneaky_mod kernel module
- sneaky_mod.c: A kernel module that intercepts system calls to hide the actions of the sneay_process program


Subversive actions:
- Modifying the /etc/passwd file (tracks every registered user that has access to the system) 
- Intercepting ls, find, cat and lsmod system commands to hide the sneaky_mod module, the sneaky_process executable and an instance of a sneaky_process program
