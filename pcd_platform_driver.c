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

struct device_config pcdev_config[] = 
{
[PCDEVA1X] = {.config_item1 = 60, .config_item2 = 21},
[PCDEVB1X] = {.config_item1 = 50, .config_item2 = 22},
[PCDEVC1X] = {.config_item1 = 40, .config_item2 = 23},
[PCDEVD1X] = {.config_item1 = 30, .config_item2 = 24}
};

struct platform_device_id pcdevs_ids[] = 
{
	{.name = "pcdev-A1x", .driver_data = PCDEVA1X},
	{.name = "pcdev-B1x", .driver_data = PCDEVB1X},
	{.name = "pcdev-C1x", .driver_data = PCDEVC1X},
	{.name = "pcdev-D1x", .driver_data = PCDEVD1X},      //this driver doesn't support d1x
	{}
};

struct of_device_id org_pcdev_dt_match[]=
{
	{.compatible = "pcdev-A1x",.data = (void*)PCDEVA1X},
	{.compatible = "pcdev-B1x",.data = (void*)PCDEVB1X},
	{.compatible = "pcdev-C1x",.data = (void*)PCDEVC1X},
	{.compatible = "pcdev-D1x",.data = (void*)PCDEVD1X},
	{}
};


/* device private data structure */
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
struct pcdrv_private_data pcdrv_data;   //global var for driver
int check_permission(int dev_perm, int acc_mode)
{

	if(dev_perm == RDWR)
		return 0;
	//ensures readonly access
	if( (dev_perm == RDONLY) && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE) ) )
		return 0;
	//ensures writeonly access
	if( (dev_perm == WRONLY) && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ) ) )
		return 0;
	return -EPERM;

}

loff_t my_llseek (struct file *filp, loff_t offset, int whence)
{
return 0;
}

ssize_t my_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
return 0;
}

ssize_t my_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
return -ENOMEM;
}
int my_open (struct inode *inode, struct file *filp)
{
return 0;
}

int my_release (struct inode *inode, struct file *filp)
{
pr_info("close was successful\n");
return 0;
}


struct file_operations my_fops = 
{
	.open = my_open,
	.write = my_write, 
	.read = my_read,
	.llseek = my_llseek,
	.release = my_release,
	.owner = THIS_MODULE
};

/*gets called when the device is removed from the system*/
int pcd_platform_driver_remove(struct platform_device *pdev)
{
#if 1
struct pcdev_private_data *dev_data = dev_get_drvdata(&pdev->dev);
/*1. Remove a device that was created with device_create() */
device_destroy(pcdrv_data.class_my, dev_data->dev_num);

/*2. REmove a cdev entry from the system */
cdev_del(&dev_data->my_cdev);
/*3. Free the memory held by the device */
//kfree(dev_data->buffer);
//kfree(dev_data);

pcdrv_data.total_devices--;
#endif
dev_info(&pdev->dev,"A device is removed\n");
return 0;
}

struct pcdev_platform_data* pcdev_get_platform(struct device* dev)
{
	struct device_node *dev_node = dev->of_node;
	struct pcdev_platform_data *pdata;
	if(!dev_node)	
	{
		/*this probe didn't happen because of device tree node*/
		return NULL;
	}
	pdata = devm_kzalloc(dev,sizeof(*pdata),GFP_KERNEL);

	if(!pdata){
		dev_info(dev,"Cannot allocate memory \n");
		return ERR_PTR(-ENOMEM);
	}
	if(of_property_read_string(dev_node,"org,device-serial-num",&pdata->serial_number) ){
		dev_info(dev,"Missing serial number property \n");
		return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node,"org,size",&pdata->size) ){
		dev_info(dev,"Missing Size property \n");
		return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node,"org,perm",&pdata->perm) ){
               dev_info(dev,"Missing permission property \n");
                return ERR_PTR(-EINVAL);
        }
	return pdata;
}
/*gets called when matched platform device is found*/
int pcd_platform_driver_probe(struct platform_device *pdev)
{
int ret;

struct pcdev_private_data *dev_data;
struct pcdev_platform_data *pdata;
struct device *dev = &pdev->dev;
int driver_data;
/* used to store matched entry of of_device_id list ofthis driver */
const struct of_device_id *match;

dev_info(dev,"A device is detected \n");
/* match will always be NULL if linux does support device tree i.e CONFIG_OF is off  */
match = of_match_device(of_match_ptr(org_pcdev_dt_match),dev);

if(match)
{
pdata = pcdev_get_platform(dev);

if(IS_ERR(pdata))
	return PTR_ERR(pdata);
driver_data = (int)match->data;
}
else
{

	/* Get the platform data*/
	pdata = (struct pcdev_platform_data*)dev_get_platdata(dev);
	driver_data = pdev->id_entry->driver_data;
}
if(!pdata)
{
    	dev_info(dev,"No platform data available\n");
    	return -EINVAL;
}

/*2. Dynamically allocate memory for the device private data*/
//dev_data = kzalloc(sizeof(*dev_data), GFP_KERNEL);
dev_data = devm_kzalloc(&pdev->dev,sizeof(*dev_data), GFP_KERNEL);
if(!dev_data){
    dev_info(dev,"Cannot allocate memory \n");
    return -ENOMEM;
}

/*save the device private data pointer in platform device structure*/
dev_set_drvdata(&pdev->dev, dev_data);      //pdev->dev.driver_data = dev_data;
dev_data->pdata.size = pdata->size;
dev_data->pdata.perm = pdata->perm;
dev_data->pdata.serial_number = pdata->serial_number;

pr_info("Device serial number = %s\n",dev_data->pdata.serial_number );
pr_info("Device size = %d\n", dev_data->pdata.size);
pr_info("Device permission = %d\n", dev_data->pdata.perm);

pr_info("Config item 1 =%d\n",pcdev_config[driver_data].config_item1);
pr_info("Config item 2 =%d\n",pcdev_config[driver_data].config_item2);
/*3. Dynamically allocate memory for the device buffer using size information from the platform data*/
//dev_data->buffer = kzalloc(dev_data->pdata.size, GFP_KERNEL);
dev_data->buffer = devm_kzalloc(&pdev->dev,dev_data->pdata.size, GFP_KERNEL);
if(!dev_data->buffer){
    pr_info("Cannot allocate memory \n");
    return -ENOMEM;
}

/*4. Get the device number*/
dev_data->dev_num = pcdrv_data.device_num_base + pcdrv_data.total_devices;

/*5. Do cdev init and cdev add*/
cdev_init(&dev_data->my_cdev, &my_fops);

dev_data->my_cdev.owner = THIS_MODULE;
ret = cdev_add(&dev_data->my_cdev,dev_data->dev_num, 1);
if(ret < 0){
	dev_err(dev,"cdev add failed\n");
	return ret;
}
/*6. Create device file for the detected platform device */
pcdrv_data.device_my = device_create(pcdrv_data.class_my,dev,dev_data->dev_num,NULL,"mypcdev-%d",pcdrv_data.total_devices);
if(IS_ERR(pcdrv_data.device_my))
{
	dev_err(dev,"device create failued\n");
	ret = PTR_ERR(pcdrv_data.device_my);
	cdev_del(&dev_data->my_cdev); 
	return ret;
}
pcdrv_data.total_devices++;

dev_info(dev,"Probe was successfull\n");
return 0;
}

struct platform_driver pcd_platform_driver = 
{
    .probe = pcd_platform_driver_probe,
    .remove = pcd_platform_driver_remove,
    .id_table = pcdevs_ids,
    .driver = {
                .name = "pseudo-char-device",
		.of_match_table = of_match_ptr(org_pcdev_dt_match)
      }
};

#define MAX_DEVICES 10
static int __init pcd_platform_driver_init(void)
{
int ret;
/*1. Dynamically allocate a device number for MAX_DEVICES */
ret = alloc_chrdev_region(&pcdrv_data.device_num_base,0,MAX_DEVICES,"pcdevs");
if(ret < 0){
	pr_err("Alloc chrdev failed\n");
	return ret;
}
/*2. Create device clas  under /sys/class */
pcdrv_data.class_my = class_create(THIS_MODULE,"my_class");
if(IS_ERR(pcdrv_data.class_my))
{
        pr_err("class creation failed\n");
        ret = PTR_ERR(pcdrv_data.class_my);
        unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
        return ret;
}

/*3. REgister a platform driver */
platform_driver_register(&pcd_platform_driver);

pr_info("pcd platform driver loaded\n");
return 0;
}
static void __exit pcd_platform_driver_exit(void)
{
/*1. Unregister the platform driver*/
platform_driver_unregister(&pcd_platform_driver);
/*Class destroy*/
class_destroy(pcdrv_data.class_my);
/*UNregister device number for MAX_DEVICES*/
unregister_chrdev_region(pcdrv_data.device_num_base, MAX_DEVICES);
  //  platform_driver_unregister(&pcd_platform_driver);
	pr_info("pcd platform driver unloaded \n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);

//module_platform_driver(pcd_platform_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A Pseudo character device which handles multiple device");
