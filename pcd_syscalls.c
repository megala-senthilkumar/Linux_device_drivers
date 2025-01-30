#include "pcd_platform_driver_dt_sysfs.h"

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
struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

int max_size=pcdev_data->pdata.size;

loff_t temp;

pr_info("lseek requested\n");
pr_info("current value of the file position = %lld\n",filp->f_pos);

switch(whence)
{
	case SEEK_SET:
	  if((offset > max_size)||(offset < 0)){
	  return -EINVAL;}
	  filp->f_pos = offset;
	  break;
	case SEEK_CUR:
	  temp = filp->f_pos + offset;
	  if((temp > max_size) ||(temp < 0)){
	   return -EINVAL;}
	  filp->f_pos = temp;
	  break;
	case SEEK_END:
	 temp = max_size + offset;
	  if((temp > max_size) || (temp < 0)){
	   return -EINVAL;}
	   filp->f_pos = temp;
	  break;

	default:
	  return -EINVAL;
}

pr_info("New Value of the file position = %lld\n",filp->f_pos);
return filp->f_pos;

}


ssize_t my_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

int max_size=pcdev_data->pdata.size;
pr_info("read requested for %zu bytes \n",count);
pr_info("current file position = %lld\n",*f_pos);

//adjust the count
	if((*f_pos + count)>max_size)
	count = max_size-*f_pos;
//copy to user
	if(copy_to_user(buff,pcdev_data->buffer+(*f_pos),count)){
	return -EFAULT;
	}
//update the current file position
	*f_pos += count;
pr_info("Number of bytessuccessfully read = %zu\n",count);
pr_info("updated file position = %lld",*f_pos);
//return number of bytes which have been successfully read
return count;

}
ssize_t my_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

int max_size=pcdev_data->pdata.size;
pr_info("write requested for %zu bytes \n",count);
pr_info("current file position = %lld\n",*f_pos);
/*adjust the count*/
if((*f_pos + count)>max_size)
count = max_size - *f_pos;

if(!count){
pr_err("no space left on this device");
return -ENOMEM;
}
/*copy from user*/
if(copy_from_user(pcdev_data->buffer+(*f_pos),buff,count)){
return -EFAULT;
}
/*update the current file position*/
*f_pos+=count;
pr_info("Number of bytes successfully written = %zu\n",count);
pr_info("update file position = %lld",*f_pos);

/*return no of b ytes which has been successfully written*/
return count;

}

int my_open (struct inode *inode, struct file *filp)
{
int ret;
int minor_n;

struct pcdev_private_data *pcdev_data;
/*find out on which device file open wsa attempted by the user sapce*/
minor_n=MINOR(inode->i_rdev);
pr_info("minor access  = %d\n",minor_n);
/*get device's private data struct*/
pcdev_data=container_of(inode->i_cdev, struct pcdev_private_data,my_cdev);
/*to supply device private dta to other methods of the driver*/
filp->private_data = pcdev_data;

/*check permission*/
ret=check_permission(pcdev_data->pdata.perm, filp->f_mode);

(!ret)?pr_info("open was successfull\n"):pr_info("open was unsuccessfull");
return ret;

}
int my_release (struct inode *inode, struct file *filp)
{
pr_info("close was successful\n");
return 0;
}

