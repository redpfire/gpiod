
/*
 * gpiod.c
 * An GPIO daemon for detecting emergency AP entry.
 * by aika(_pikapi)
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>

int mem_fd;
int log_fd;
void *gpio_map;
volatile unsigned *gpio;

void setup_io();

#define BCM_BASE 0x3F000000
#define GPIO_BASE (BCM_BASE + 0x200000)
#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+((g)/10)) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)
#define GPIO_CLR *(gpio+10)

#define GET_GPIO(g) (*(gpio+13)&(1<<g))

#define GPIO_PULL *(gpio+37)
#define GPIO_PULLCLK0 *(gpio+38)

int check_ap()
{
	return GET_GPIO(2);
}

void open_log()
{
	log_fd = open("/tmp/gpiod.log", O_RDWR|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void _log(const char *txt)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	char buffer[2048];
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "[%d-%d-%d %d:%d:%d] %s\n", tm.tm_year + 1900, tm.tm_mon +
			1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, txt);
	write(log_fd, buffer, strlen(buffer));
}

void daemonize()
{	
	int i;
	INP_GPIO(2);
	OUT_GPIO(2);

	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
    tm.tm_min += 5;
    time_t start_time = mktime(&tm);
	
	for(;;)
	{
        t = time(NULL);
        if (t >= start_time)
            break;

		if(check_ap())
		{
			_log("Starting emergency hotspot and exiting.");
			execv("bash -c \"/etc/gpiod/start.sh &> /tmp/gpiod-startup.log\"");
		}
		usleep(50000);
	}
    _log("Time exceeded. Exiting.");
	
	close(log_fd);
}

int main(int argc, char** argv)
{
    if(argc < 2) { return -1; }
	int pid, pidfile;

	setup_io();
	open_log();

	if((pid = fork()) == 0)
	{
		daemonize();
	}
	else
	{
		char buf[1024];
		sprintf(buf, "Spawning daemon on %d", pid);
		_log(buf);
		pidfile = open(argv[1], O_RDWR|O_CREAT, S_IRUSR | S_IWUSR
				| S_IRGRP | S_IROTH);
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%d", pid);
		write(pidfile, buf, strlen(buf));
		close(pidfile);

	}
	return 0;
}

void setup_io()
{
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
	{
		printf("can't open /dev/mem. Are you root?\n");
		exit(1);
	}

	gpio_map = mmap(
		NULL,
		BLOCK_SIZE,
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		mem_fd,
		GPIO_BASE
	);

	close(mem_fd);

	if (gpio_map == MAP_FAILED)
	{
		printf("mmap error %d\n", (int)gpio_map);
		exit(-1);
	}

	gpio = (volatile unsigned *)gpio_map;
}

