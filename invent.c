#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#define FIRST_ENTRY_OFFSET	2048
#define ENTRY_SIZE		2048
struct invent_entry {
	char part_number[9]; // length 8, offset 0
	char bin_location[13]; // length 12, offset 73
	char bin_alt1[13]; // length 12, offset 85
	char bin_alt2[13]; // length 12, offset 97
};
char record[ENTRY_SIZE];
int main()
{
	// first entry at 2048 bytes
	FILE *fp = fopen("INVENT", "rb");
	if(fp == NULL)
	{
		printf("Error opening INVENT: %s\n", strerror(errno));
		return 1;
	}
	fseek(fp, FIRST_ENTRY_OFFSET, SEEK_SET);
	if(ftell(fp) != FIRST_ENTRY_OFFSET)
	{
		printf("fseek failed: %s\n", strerror(errno));
		return 1;
	}
	while(1)
	{
		// part number is 8 chars long max
		struct invent_entry entry = {0};
		int pos = ftell(fp);
		if(!fread(record, ENTRY_SIZE, 1, fp))
		{
			if(feof(fp))
				break;
			printf("fgets failed: %s\n", strerror(errno));
			return 1;
		}
		#define entry_copy(field, record) do{memcpy((field), record, sizeof(field)-1);(field)[sizeof(field)-1]='\0';}while(0)
		entry_copy(entry.part_number, record);
		entry_copy(entry.bin_location, record+73);
		entry_copy(entry.bin_alt1, record+85);
		entry_copy(entry.bin_alt2, record+97);
		if(record[0]=='\xFF')
		{
			// seems to be deleted records
			continue;
		}
		else if(record[0]=='\x00')
		{
			// blank entries in DB?
			continue;
		}
		//printf("%d: %s\n", pos, record);
		printf("%s\n", entry.bin_location);
	}
	fclose(fp);
	return 0;
}
