#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <getopt.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#define FIRST_ENTRY_OFFSET	2048
#define ENTRY_SIZE		2048
struct invent_entry {
	char part_number[21]; // length 20, offset 0
	char desc[11]; //length 11, offset 58
	char bin_location[13]; // length 12, offset 73
	char bin_alt1[13]; // length 12, offset 85
	char bin_alt2[13]; // length 12, offset 97
	char entry_date[9]; // length 8, offset 148
	char ext_desc[61]; // length 60, offset 1556
};

struct hbsdb_file_def {
	int record_size;
	int first_record_offset;
	char *filename;
};
struct hbsdb_file_def invdesc = {
	.record_size = 512,
	.first_record_offset = 512,
	.filename = "INVDESC"
};

struct invdesc_entry {
	#if 1
	#endif
	char part_number[21]; // length 20, offset 0
	char comment1[15]; // length TODO, offset 44
};
char record[ENTRY_SIZE];

void print_usage(char *argv0)
{
	printf("Usage: %s [filename]\n", argv0);
}

void print_entry(struct invent_entry *entry)
{
	printf("%s: %s\n", entry->part_number, entry->bin_location);
	if(strlen(entry->desc) > 0)
		printf("%s\n", entry->desc);
	if(strlen(entry->ext_desc) >0)
		printf("%s\n", entry->ext_desc);
	printf("\n");
}

int main(int argc, char *argv[])
{
	char *filename = "INVENT";
	char *to_find = NULL;
	struct option long_options[] =
	{
		{"help",	no_argument,		0,	'h'},
		{"find",	required_argument,	0,	'f'},
		{"all",		no_argument,		0,	'a'},
		{"regex",	no_argument,		0,	'r'},
		{0,0,0,0}
	};
	int option_index = 0;
	int c;
	int print_all_flag = 0;
	pcre2_code *regex = NULL;
	int errorcode = 0;
	PCRE2_SIZE erroroffset = 0;
	int ret = 0;
	int do_regex = 0;
	while(1)
	{
		c = getopt_long(argc, argv, "hf:ar", long_options, &option_index);
		if(c == -1)
			break;
		switch(c)
		{
			case 'h':
				print_usage(argv[0]);
				return 0;
			case 'f':
				to_find = optarg;
				break;
			case 'a':
				print_all_flag = 1;
				break;
			case 'r':
				do_regex = 1;
				break;
		}
	}
	if(optind < argc)
	{
		filename = argv[optind++];
	}
	if(optind < argc)
	{
		print_usage(argv[0]);
		return 0;
	}
	
	if(do_regex)
	{
		// If we have a regex, compile it and check for errors
		// case-insensitive
		regex = pcre2_compile((PCRE2_SPTR)to_find, PCRE2_ZERO_TERMINATED, PCRE2_CASELESS, &errorcode, &erroroffset, NULL);
		if(regex == NULL)
		{
			PCRE2_UCHAR buffer[256];
			pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
			printf("Regex error at offset %d: %s\n", (int)erroroffset, buffer);
			return 1;
		}
	}

	// first entry at 2048 bytes
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL)
	{
		printf("Error opening %s: %s\n", filename, strerror(errno));
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
		entry_copy(entry.desc, record+58);
		entry_copy(entry.bin_location, record+73);
		entry_copy(entry.bin_alt1, record+85);
		entry_copy(entry.bin_alt2, record+97);
		entry_copy(entry.entry_date, record+148);
		entry_copy(entry.ext_desc, record+1556);
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
		if(to_find)
		{
			if(!strcmp(entry.part_number, to_find))
			{
				print_entry(&entry);
				continue;
			}
		}
		if(to_find && regex)
		{
			pcre2_match_data *matchdata;
			matchdata = pcre2_match_data_create_from_pattern(regex, NULL);
			ret = pcre2_match(regex, (PCRE2_SPTR)entry.part_number, PCRE2_ZERO_TERMINATED, 0, 0, matchdata, NULL);
			if(ret > 0)
			{
				print_entry(&entry);
			}
		}
		if(print_all_flag)
			printf("%s\n", entry.part_number);
	}
	fclose(fp);
	return 0;
}
