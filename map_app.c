#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#define size 4096
#define file "/dev/mymap"

int main(int argc, char *argv[])
{
	int fd=-1;
	int i=0;
	unsigned char *p_map=NULL;

	fd = open(file,O_RDWR);
	if(fd<0)
	{
		printf("open mymap error.\n");
		return -1;
	}
	
	p_map = (unsigned char *)mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	if(MAP_FAILED == p_map)
	{
		printf("p_map error.\n");
	}

	for(i=0; i<10; i++)
	printf("the mmap = %c.\n",p_map[i]);
	munmap(p_map, size);
	return 0;
}
/*
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <fcntl.h> 
#include <linux/fb.h> 
#include <sys/mman.h> 
#include <sys/ioctl.h> 

#define SIZE 4096 


int main(int argc , char *argv[]) 
{ 
int fd; 
int i; 
unsigned char *p_map=NULL; 

//打开设备 
fd = open("/dev/mymap",O_RDWR); 
if(fd < 0) 
{ 
printf("open fail\n"); 
exit(1); 
} 

//内存映射 
p_map = (unsigned char *)mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,fd, 0); 
if(p_map == MAP_FAILED) 
{ 
printf("mmap fail\n"); 
goto here; 
} 

//打印映射后的内存中的前10个字节内容 
for(i=0;i<10;i++) 
printf("%c\n",p_map[i]); 


here: 
munmap(p_map, SIZE); 
return 0; 
} 
*/








