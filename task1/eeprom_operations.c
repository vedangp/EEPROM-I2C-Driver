#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <errno.h>

#define PAGE_SIZE		64
#define NUMBER_OF_PAGES		512
#define EEPROM_I2C_ADDRESS	0x52
#define EEPROM_I2C_READ_CMD	((EEPROM_I2C_ADDRESS) | 0x01)
#define EEPROM_I2C_WRITE_CMD	((EEPROM_I2C_ADDRESS) & ~(0x01))

int eeprom_i2c_fd;
int current_seek_address	    = 0;
int current_seek_address_high	    = 0;
int current_seek_address_low	    = 0;

/*
 * Open the file pointing to i2c driver.
 */
int init_EEPROM()
{
	int ret;
	eeprom_i2c_fd = open("/dev/i2c-2",O_RDWR);

	ret = ioctl(eeprom_i2c_fd,I2C_SLAVE,EEPROM_I2C_ADDRESS);
	if (ret < 0)
	{
		printf("ERROR: %s\n",strerror(errno));
	}
}

/*
 * tries to write count*PAGE_SIZE pages to EEPROM. Returns 0 in case of success, -1 in case of faliure
 */
int write_EEPROM(void *buf,int count)
{
	int i,j;
	char *temp;
	char* temp2 = buf;
 
	/* allocating memory to temp which can store the buffer + high and low byte of address */
	temp = (char*) malloc (PAGE_SIZE + 2);
	
	/* writing to eeprom page-by-page */
	for (i =0;i<count;i++)
	{
		int ret;
		temp[0] = current_seek_address_high;
		temp[1] = current_seek_address_low;    
		for (j =0;j<PAGE_SIZE;j++)
		{
			temp[2+j] =  temp2[j + i*PAGE_SIZE];
		}
		ret = write(eeprom_i2c_fd,temp,PAGE_SIZE+2);
		
		if (ret < 0)
		{
			printf("ERROR: %s\n",strerror(errno));
			return -1;
		}
		sleep(1);
		printf("Page %d written\n", current_seek_address);
		seek_EEPROM((current_seek_address + 1) % NUMBER_OF_PAGES);
	}
	return 0;
}


/*
 * tries to read count*PAGE_SIZE pages to EEPROM. Returns 0 in case of success, -1 in case of faliure
 * buffer size should be equal to count * PAGE_SIZE.
 */
int read_EEPROM(void *buf,int count)
{
	int ret =0;
	char address[2];
	
	/* writing address from where the read will start */
	address[0] = current_seek_address_high;
	address[1] = current_seek_address_low;
	ret = write(eeprom_i2c_fd,address, sizeof(char) * 2); 
	if(ret < 0)
	{
		printf("ERROR: %s\n",strerror(errno));
		ret = -1;
	}

	/* reading the data */
	ret = read(eeprom_i2c_fd,buf,count*PAGE_SIZE);
	if(ret < 0)
	{
		printf("ERROR: %s\n",strerror(errno));
		ret = -1;
	}
	
	seek_EEPROM((current_seek_address + count) % NUMBER_OF_PAGES);
	return ret;
}

/*
 * tries to set seek address of EEPROM. Returns 0 in case of success, -1 in case of faliure
 */
int seek_EEPROM(int offset)
{
	if (offset > 511)
		return -1;
	current_seek_address = offset;
	current_seek_address_high = (((current_seek_address << 6) & 0xFF00) >> 8);
	current_seek_address_low  = ((current_seek_address << 6) & 0x00FF);
	return 0;
}
