#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int main()
{
	int fd = open("INVENT", O_RDWR);
	if(fd < 0)
	{
		printf("open() failed: %s\n", strerror(errno));
		return 1;
	}
	struct stat s = {0};
	int ret = fstat(fd, &s);
	if(ret < 0)
	{
		printf("fstat() failed: %s\n", strerror(errno));
		return 1;
	}
	char *map;
	map = mmap(NULL, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(map == MAP_FAILED)
	{
		printf("mmap()ing failed: %s\n", strerror(errno));
		return 1;
	}
	close(fd);
	// We've got the mapping. Let's have fun.
	const int record_size = 2048;
	size_t idx = record_size; // Skip the first record; it isn't real.
	const int source_offset = 53;
	const int source_len = 3;
	// TODO If source is greater than 800 (not equal), set vendor = 800
	do
	{
		if(!strncmp(map+idx+source_offset, "840", 3))
		{
			char partno[21];
			memcpy(partno, map+idx, 20);
			partno[20] = 0;
			printf("Found one with 840: %s\n", partno);
			*(map+idx+0) = '8';
			*(map+idx+1) = '0';
			*(map+idx+2) = '0';
		}
		idx += record_size;
	} while(idx < s.st_size);
	return 0;
}
