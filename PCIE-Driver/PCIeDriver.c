#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/cdev.h>

#define CLASS_NAME  "myClass"
#define DEVICE_NAME "myDevice"


static const struct pci_device_id my_pci_ids[] = {
    { PCI_DEVICE(0x1234, 0x11e8) },   //Intended for qemu emulation, swap with your own Device ID and Vendor ID
    { 0, }
};
MODULE_DEVICE_TABLE(pci, my_pci_ids);


struct pcie_suitcase {
    struct cdev cdev;
    dev_t dev_t;
    struct device *dev;
    void __iomem *bar;
};

static struct class *chr_class;


static int my_open(struct inode *inodep, struct file *fp){
    struct pcie_suitcase *priv = container_of(inodep->i_cdev, struct pcie_suitcase, cdev);
    fp->private_data = priv;
    return 0;
}

static ssize_t my_read(struct file *fp, char __user *buffer, size_t len,loff_t *offset){
    struct pcie_suitcase *p = fp->private_data;
    u32 val = ioread32(p->bar);

    if (copy_to_user(buffer, &val, sizeof(val)))
        return -EFAULT;

    return sizeof(val);
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open  = my_open,
    .read  = my_read,
};


static int my_probe(struct pci_dev *pdev, const struct pci_device_id *id){
    struct pcie_suitcase *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(struct pcie_suitcase), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    ret = pci_enable_device(pdev);
    if (ret)
        return ret;

    ret = pci_request_regions(pdev, DEVICE_NAME);
    if (ret) {
        pci_disable_device(pdev);
        return ret;
    }

    priv->bar = pci_iomap(pdev, 0, 0);
    if (!priv->bar) {
        ret = -ENOMEM;
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        return ret;
    }

    ret = alloc_chrdev_region(&priv->dev_t, 0, 1, DEVICE_NAME);
    if (ret) {
        pci_iounmap(pdev, priv->bar);
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        return ret;
    }

    cdev_init(&priv->cdev, &fops);
    priv->cdev.owner = THIS_MODULE;

    ret = cdev_add(&priv->cdev, priv->dev_t, 1);
    if (ret) {
        unregister_chrdev_region(priv->dev_t, 1);
        pci_iounmap(pdev, priv->bar);
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        return ret;
    }

    priv->dev = device_create(chr_class, &pdev->dev, priv->dev_t, NULL, DEVICE_NAME "%d", MINOR(priv->dev_t));
    if (IS_ERR(priv->dev)) {
        ret = PTR_ERR(priv->dev);
        cdev_del(&priv->cdev);
        unregister_chrdev_region(priv->dev_t, 1);
        pci_iounmap(pdev, priv->bar);
        pci_release_regions(pdev);
        pci_disable_device(pdev);
        return ret;
    }

    pci_set_drvdata(pdev, priv);
    return 0;
}


static void my_remove(struct pci_dev *pdev){
    struct pcie_suitcase *p = pci_get_drvdata(pdev);

    device_destroy(chr_class, p->dev_t);
    cdev_del(&p->cdev);
    unregister_chrdev_region(p->dev_t, 1);

    pci_iounmap(pdev, p->bar);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
}


static struct pci_driver my_driver = {
    .name = "my_pcie_driver",
    .id_table = my_pci_ids,
    .probe = my_probe,
    .remove = my_remove,
};


static int __init my_init(void){
    chr_class = class_create(CLASS_NAME);
    if (IS_ERR(chr_class))
        return PTR_ERR(chr_class);

    return pci_register_driver(&my_driver);
}

static void __exit my_exit(void){
    pci_unregister_driver(&my_driver);
    class_destroy(chr_class);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");

