#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

int count1 = 0; 

void fill_data(char *buf,int count)
{
	int i;
	
	for(i=0;i<count*64;i++)
	{
		buf[i] = i;
	}
}

void print_data(char *buf,int count)
{
	int i;
	
	for(i=0;i<count*64;i++)
	{
		printf("%d\n",buf[i]);
	}
}

int main()
{
	int add;
	char i;
	char *buf;
	int ret, errsv;
	int i2c_dev_fd;
	int data_count;
	
	i2c_dev_fd = open("/dev/i2c_flash",O_RDWR);
	if(i2c_dev_fd < 0)
		printf("Error: %s\n",strerror(errno));
	printf("Usage:\n\tw: write\n\tr: read\n\ts: seek\nr & w will read and write 2 pages of data.\n");

	i = getchar();	
	
	
	lseek(i2c_dev_fd,0,SEEK_SET);
	
	
	while(1)
	{
		switch(i)
		{
			case 'w':
				printf("Enter the number of pages you want to write:\n");
				scanf("%d",&data_count);
				buf = (char*)calloc(64*data_count,sizeof(char));
				fill_data(buf,data_count);	
				ret = write(i2c_dev_fd,buf,data_count);
				errsv = errno;
				while(ret != 0 && errsv != 3)
				{
					ret = write(i2c_dev_fd,buf,2);
					errsv = errno;
				}
				if (ret < 0) printf("Error\n");
				free(buf);
				break;
			case 'r':
				printf("Enter the number of pages you want to read:\n");
				scanf("%d",&data_count);
				buf = (char*)calloc(64*data_count,sizeof(char));
				ret = read(i2c_dev_fd,buf,data_count);
				errsv = errno;
				while (ret != 0)
				{
					ret = read(i2c_dev_fd,buf,data_count);
					errsv = errno;
				}
				if (ret < 0) 
					printf("Error: %s\n",strerror(errno));
				else
					print_data((void*)buf,data_count);
				free(buf);
			    	break;
			case 's':   
				printf("enter where you want to seek:\n");
				scanf("%d",&add);
				lseek(i2c_dev_fd,add,SEEK_SET);
				if (ret == -1) 
					printf("Error: %s\n",strerror(errno));
				break;
			case 'e': 
				exit(0);
				break;

		}
		i = getchar();	
	}
}
