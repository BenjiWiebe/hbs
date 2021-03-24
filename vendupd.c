#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>

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
	const int vendor_offset = 30;
	const int source_len = 3;
	const int vendor_len = 3;
	char *ptr = map;
	// If source is greater than or equal to 800, and vendor is 8xx, set vendor = 800
	do
	{
		char source[4], vendor[4];
		memcpy(source, ptr+source_offset, source_len);
		memcpy(vendor, ptr+vendor_offset, vendor_len);
		source[3] = vendor[3] = 0;
		int sourcei = atoi(source);
		int vendori = atoi(vendor);
		if(sourcei >= 800 && sourcei < 900 && vendori > 800 && vendori < 900)
		{
			if(ptr[0] == '\xFF')
			{
				ptr += record_size;
				continue;
			}
			char partno[21];
			strncpy(partno, ptr, 20);
			partno[20] = 0;
			printf("Found one with source %d, vendor %d: %s\n", sourcei, vendori, partno);
			(ptr+vendor_offset)[0] = '8';
			(ptr+vendor_offset)[1] = '0';
			(ptr+vendor_offset)[2] = '0';
		}
		ptr += record_size;
	} while(ptr < s.st_size + map);
	return 0;
}
