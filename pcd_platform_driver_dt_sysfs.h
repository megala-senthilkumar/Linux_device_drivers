#ifndef PCD_PLATFORM_DRIVER_DT_SYSFS_H
#define PCD_PLATFORM_DRIVER_DT_SYSFS_H

#include<linux/module.h>
#include<linux/fs.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/kdev_t.h>	//MAJOR & MINOR Macro device id
#include<linux/uaccess.h>	//
#include<linux/platform_device.h>
#include<linux/slab.h>
#include<linux/mod_devicetable.h>
#include<linux/of.h>
#include<linux/of_device.h>

#include"platform.h"

#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt,__func__

int check_permission(int dev_perm, int acc_mode);
loff_t my_llseek (struct file *filp, loff_t offset, int whence);
ssize_t my_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t my_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int my_open (struct inode *inode, struct file *filp);
int my_release (struct inode *inode, struct file *filp);

struct device_config
{
int config_item1;
int config_item2;
};

enum pcev_names
{
PCDEVA1X,
PCDEVB1X,
PCDEVC1X,
PCDEVD1X
};

struct pcdev_private_data
{
        struct pcdev_platform_data pdata;
        char *buffer;
        dev_t dev_num;
        struct cdev my_cdev;
};

/* driver private data structure  */
struct pcdrv_private_data
{
        int total_devices;
	    dev_t device_num_base;
	    struct class *class_my;
        struct device *device_my;
};

#endif
