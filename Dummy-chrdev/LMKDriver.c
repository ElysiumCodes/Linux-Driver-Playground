#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/err.h>

#define CLASS_NAME  "myclass"
#define DEVICE_NAME "myDevice"

char message[100] = "Hello from the Kernel!\n";

MODULE_LICENSE("GPL");

static int major_number;
static struct class  *chr_class  = NULL;
static struct device *chr_dev    = NULL;

static int myOpen(struct inode *inodep, struct file *fp){
    static int counter = 0;
    counter++;
    printk(KERN_INFO "Your module has been opened %d times", counter);
    return 0;
}

static ssize_t myRead(struct file *fp, char __user *buffer,size_t len, loff_t *offset){
    int sizeofmsg = 24;

    if (*offset > 0) return 0;

    if (copy_to_user(buffer, message, sizeofmsg))
        return -EFAULT;

    *offset += sizeofmsg;
    return sizeofmsg;
}

static ssize_t myWrite(struct file *fp, const char __user *buffer,size_t count, loff_t *offset){
    char kernelBuffer[100];
    size_t size = min(count, sizeof(kernelBuffer) - 1);

    if (copy_from_user(kernelBuffer, buffer, size))
        return -EFAULT;

    kernelBuffer[size] = 0;
    printk(KERN_INFO "Data written to hello driver");

    return size;
}

static struct file_operations fops = {
    .open  = myOpen,
    .read  = myRead,
    .write = myWrite,
    .owner = THIS_MODULE,
};

static int __init hello_init(void){
    printk(KERN_INFO "Initializing Loadable Kernel Module\n");

    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "Character Device not registered");
        return major_number;
    }

    chr_class = class_create(CLASS_NAME);
    if (IS_ERR(chr_class)){
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Device class not created");
        return PTR_ERR(chr_class);
    }

    chr_dev = device_create(chr_class, NULL,MKDEV(major_number, 0),NULL, DEVICE_NAME);
    if (IS_ERR(chr_dev)) {
        class_destroy(chr_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Device Node not created");
        return PTR_ERR(chr_dev);
    }

    return 0;
}

static void __exit hello_exit(void){
    printk(KERN_INFO "Exiting Kernel Module now...\n");

    device_destroy(chr_class, MKDEV(major_number, 0));
    class_destroy(chr_class);
    unregister_chrdev(major_number, DEVICE_NAME);
}

module_init(hello_init);
module_exit(hello_exit);

