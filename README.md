# Dummy-Linux-Driver
My first attempt at writting Linux device driver modules. The driver provides methods for various syscalls. 

- run make to compile the project
- run sudo insmod LMKDriver.ko to load the kernel module
- Verify that its been added with lsmod and sudo cd /dev/myDevice
- run sudo cat /dev/myDevice and echo "Your text here" | sudo tee /dev/myDevice to verify read and write respectively
- check kernel logs at sudo dmesg
