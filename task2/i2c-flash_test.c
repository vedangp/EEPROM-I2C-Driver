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
	int ret;
	int i2c_dev_fd;
	
	i2c_dev_fd = open("/dev/i2c_flash",O_RDWR);
	if(i2c_dev_fd < 0)
		printf("Error: %s\n",strerror(errno));
	i = getchar();	
	buf = (char*)calloc(128,sizeof(char));
	
	lseek(i2c_dev_fd,0,SEEK_SET);

	while(1)
	{
		switch(i)
		{
			case 'w':
				fill_data(buf,2);	
				ret = write(i2c_dev_fd,buf,2);
				if (ret < 0) printf("Error: %s\n",strerror(errno));
				break;
			case 'r':
				ret = read(i2c_dev_fd,buf,2);
				if (ret < 0) 
					printf("Error: %s\n",strerror(errno));
				else
					print_data((void*)buf,2);
			    	break;
			case 's':   
				scanf("%d",&add);
				lseek(i2c_dev_fd,add,SEEK_SET);
				if (ret == -1) 
					printf("Error: %s\n",strerror(errno));
				break;
			case 'e': 
				exit(0);
				break;
			default:
				printf("Invalid Input \n");
				break;
		}
		i = getchar();	
	}
}
