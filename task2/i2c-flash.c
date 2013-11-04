#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/param.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/delay.h>

#define SLAVE_ADDRESS 			0x52
#define DEVICE_NAME 			"i2c_flash"
#define I2C_DEVICE_ID 			2
#define EEPROM_PAGE_SIZE		64
#define NUMBER_OF_PAGES			512
#define EEPROM_I2C_ADDRESS		0x52

struct I2c_flash{
	struct cdev cdev;
	char name[20];
	int offset;
	int current_seek_address;
	int current_seek_address_high;
	int current_seek_address_low;

	struct i2c_client *client;
} *i2c_flash_devp;
static dev_t i2c_flash_devp_number;
struct class *i2c_flash_class;

/* Helper Functions */

/*
 * tries to set seek address of EEPROM. Returns 0 in case of success, -1 in case of faliure
 */
int seek_EEPROM(int offset,struct I2c_flash *i2c_flash_devp)
{
	if (offset > 511)
		return -1;
	i2c_flash_devp->current_seek_address = offset;
	i2c_flash_devp->current_seek_address_high = (((i2c_flash_devp->current_seek_address << 6) & 0xFF00) >> 8);
	i2c_flash_devp->current_seek_address_low  = ((i2c_flash_devp->current_seek_address << 6) & 0x00FF);
	return 0;
}

/* ----------------------------------------------------------------*/

/* open i2c_flash */
int i2c_flash_open (struct inode* inode, struct file* file)
{
	struct I2c_flash *i2c_flash_devp;
	
	i2c_flash_devp = container_of(inode->i_cdev,struct I2c_flash,cdev);
	file->private_data = i2c_flash_devp;

	return 0;
}

/* close i2c_flash */
int i2c_flash_release (struct inode* inode, struct file* file) 
{

	file->private_data = NULL;

	return 0;
}
/* ------------------------------------------------------------------------------------ */
/* write 1 page to i2c_flash */
ssize_t i2c_flash_write(struct file* file,  const char *buf, size_t count, loff_t * ppos)
{
	int i,j;
	char *temp;
	char *temp2;
	struct I2c_flash *i2c_flash_devp = file->private_data;
	struct i2c_client *client = i2c_flash_devp->client;
	/* allocating memory to temp which can store the buffer + high and low byte of address */
	temp = (char*) kmalloc (EEPROM_PAGE_SIZE + 2, GFP_KERNEL);
	temp2= (char*) kmalloc (count*EEPROM_PAGE_SIZE, GFP_KERNEL);
	
	if (copy_from_user(temp2, buf, count*EEPROM_PAGE_SIZE))
	{
		kfree(temp);
		kfree(temp2);
		return -EFAULT;
	}	
	/* writing to eeprom page-by-page */
	for (i =0;i<count;i++)
	{
		int ret;
		temp[0] = i2c_flash_devp->current_seek_address_high;
		temp[1] = i2c_flash_devp->current_seek_address_low;    
		for (j =0;j<EEPROM_PAGE_SIZE;j++)
		{
			temp[2+j] =  temp2[j + i*EEPROM_PAGE_SIZE];
		}
		ret = i2c_master_send (client,temp,EEPROM_PAGE_SIZE + 2);
		
		if (ret < 0)
		{
			kfree(temp);
			kfree(temp2);
			return ret;
		}
		msleep(10);
		printk("Page %d written\n", i2c_flash_devp->current_seek_address);
		seek_EEPROM((i2c_flash_devp->current_seek_address + 1) % NUMBER_OF_PAGES,i2c_flash_devp);
	}
	kfree(temp);
	kfree(temp2);
	return 0;

}

/* read 1 pageo from i2c_flash */
ssize_t i2c_flash_read(struct file* file, char __user *buf, size_t count, loff_t * ppos)
{
	int ret =0;
	char address[2];
	char *temp;
	
	struct I2c_flash *i2c_flash_devp = file->private_data;
	struct i2c_client *client = i2c_flash_devp->client;
	
	temp = (char*) kmalloc(count*EEPROM_PAGE_SIZE,GFP_KERNEL);
	
	/* writing address from where the read will start */
	address[0] = i2c_flash_devp->current_seek_address_high;
	address[1] = i2c_flash_devp->current_seek_address_low;
	ret = i2c_master_send(client,address,sizeof(char)*2);
	if(ret < 0)
	{
		kfree(temp);
		return ret;
	}

	/* reading the data */
	ret = i2c_master_recv(client,temp,count*EEPROM_PAGE_SIZE);
	if(ret < 0)
	{
		kfree(temp);
		return ret;
	}
	
	if(copy_to_user(buf,temp,count*EEPROM_PAGE_SIZE))
	{
		kfree(temp);
		return -EFAULT;
	}
	
	seek_EEPROM((i2c_flash_devp->current_seek_address + count) % NUMBER_OF_PAGES,i2c_flash_devp);
	kfree(temp);
	return ret;
}

/* set the page number to be read next */
loff_t i2c_flash_seek(struct file* file, loff_t offset, int whence)
{
	struct I2c_flash *i2c_flash_devp = file->private_data;
	switch(whence)
	{
		case SEEK_SET:
			seek_EEPROM(offset,i2c_flash_devp);
	}
	return 0;
}

/* ------------------------------------------------------------------------ */

static struct file_operations f_ops= {
	.owner  	= THIS_MODULE,
	.open 		= i2c_flash_open,
	.release	= i2c_flash_release,
	.write		= i2c_flash_write,
	.read 		= i2c_flash_read,
	.llseek 	= i2c_flash_seek,
};

/* ----------------------------------------------------------------------- */
int setup_i2c_client(void)
{
	struct i2c_adapter *adap;
	struct i2c_board_info *i2c_info;
	unsigned short address = SLAVE_ADDRESS;
	
	i2c_info = (struct i2c_board_info*) kmalloc(sizeof(struct i2c_board_info), GFP_KERNEL);
	
	adap = i2c_get_adapter(I2C_DEVICE_ID);
	if (!adap)
		return -ENODEV;
	
	i2c_flash_devp->client = i2c_new_probed_device(adap, i2c_info, &address, NULL);
	if (!(i2c_flash_devp->client)) {
		i2c_put_adapter(adap);
		return -ENOMEM;
	}
	return 0;
}
/* ----------------------------------------------------------------------- */
/* i2c_flash init */
static int __init i2c_flash_init(void)
{
	int ret;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&i2c_flash_devp_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	i2c_flash_devp = kmalloc(sizeof(struct I2c_flash),GFP_KERNEL);

	/* Populate sysfs entries */
	i2c_flash_class = class_create(THIS_MODULE, DEVICE_NAME);
	
	sprintf(i2c_flash_devp->name, DEVICE_NAME);  

	/* Connect the file operations with the cdev */
	cdev_init(&i2c_flash_devp->cdev, &f_ops);
	i2c_flash_devp->cdev.owner = THIS_MODULE;

	ret = setup_i2c_client();
	
	if (ret < 0)
	{
		return ret;
	}

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&i2c_flash_devp->cdev, (i2c_flash_devp_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}
	
	/* Send uevents to udev, so it'll create /dev nodes */
	device_create(i2c_flash_class, NULL, MKDEV(MAJOR(i2c_flash_devp_number), 0), NULL, DEVICE_NAME);		

	printk("%s Initialized\n",i2c_flash_devp->name);
	
	return 0;
}

/* i2c_flash exit */ 
static void __exit i2c_flash_exit(void)
{
	unregister_chrdev_region((i2c_flash_devp_number), 1);
	
	i2c_put_adapter(i2c_flash_devp->client->adapter);
	
	i2c_unregister_device(i2c_flash_devp->client);
	
	/* Destroy device */
	device_destroy (i2c_flash_class, MKDEV(MAJOR(i2c_flash_devp_number), 0));
	cdev_del(&i2c_flash_devp->cdev);

	/* Freeing the device specific structs */
	kfree(i2c_flash_devp);

	/* Destroy driver_class */
	class_destroy(i2c_flash_class);

	printk("%s removed.\n",i2c_flash_devp->name);
}

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Vedang Patel <vedang.patel@asu.edu>");

module_init(i2c_flash_init);
module_exit(i2c_flash_exit);
