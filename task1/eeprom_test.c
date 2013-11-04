#include <stdio.h>
#include <stdlib.h>
#include "eeprom_operations.c"

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

	init_EEPROM();
	i = getchar();	
	buf = (char*)calloc(128,sizeof(char));
	
	seek_EEPROM(0);

	while(1)
	{
		switch(i)
		{
			case 'w':
				fill_data(buf,2);	
				ret = write_EEPROM((void*)buf,2);
				if (ret == -1) printf("Error\n");
				break;
			case 'r':
				ret = read_EEPROM((void*)buf,2);
				if (ret == -1) 
					printf("Error\n");
				else
					print_data((void*)buf,2);
			    	break;
			case 's':   
				scanf("%d",&add);
				seek_EEPROM(add);
				if (ret == -1) 
					printf("Error\n");
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
