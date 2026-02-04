# Dummy-Linux-Driver
My playground/storage of the dummy Linux drivers I write for practice. The driver provides methods for various syscalls. 

- run make to compile the project
- run sudo insmod [Drivername].ko to load the kernel module
- Verify that its been added with lsmod and sudo cd /dev/myDevice
- check kernel logs at sudo dmesg
