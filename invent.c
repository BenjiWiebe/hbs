#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <getopt.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "normalize.h"
#include "json_stringify.h"

#define INVENT_FIRST_ENTRY_OFFSET	2048
#define INVENT_ENTRY_SIZE		2048

#define INVDESC_FIRST_ENTRY_OFFSET	512
#define INVDESC_ENTRY_SIZE		512

// Maximum number of -f <partno> to store from the command line
#define MAX_PART_NUMBERS	64

#define MONTH_HIST_LEN	48
#define YEAR_HIST_LEN	9

// If something goes wrong on an architecture other than x86/x86_64 or with a non-GCC compiler, refactor the code to just copy the double in a loop rather than doing fancy un-portable packed struct memcpy's.
struct padded_double {
	unsigned char padding[6];
	double value;
} __attribute__((packed));
struct invent_entry {
	char part_number[21]; // length 20, offset 0
	char status[2]; // length 1, offset 52. A=Active, O=Obsolete
	int price; // length 8, offset 188. price in cents
	int cost; // length 8, offset 180, manufacturer cost in cents
	double on_hand; //length 8, offset 458.
	char vendor[4]; // length 3, offset 30
	char supplier[4]; // length 3, offset 53
	char mfrid[6]; // length 6, offset 36
	char desc[11]; //length 10, offset 58
	char bin_location[13]; // length 12, offset 73
	char bin_alt1[13]; // length 12, offset 85
	char bin_alt2[13]; // length 12, offset 97
	char entry_date[9]; // length 8, offset 148
	char ext_desc[61]; // length 60, offset 1556
	// offset starts at 298, length sizeof(double). 
	// offset increments by 14
	struct padded_double month_history[MONTH_HIST_LEN]; // 48 months of sales history
	// offset starts at 970
	// offset increments by 14
	struct padded_double year_history[YEAR_HIST_LEN]; // 9 years of sale history
	bool has_comments;
	char note1[1];
	char note2[1];
};

char record[INVENT_ENTRY_SIZE];

void print_usage(char *argv0)
{
	printf("Usage: %s [filename]\n", argv0);
	printf("  -h,--help             Print this message.\n");
	printf("  -f,--find <partno>    Find information for <partno>.\n");
	printf("  -a,--all              Print information for all parts.\n");
	printf("  -r,--regex <pattern>  Find parts matching <pattern>.\n");
	printf("     --search-desc      Check description against regex.\n");
	printf("     --search-extdesc   Check extended description against regex.\n");
	printf("     --match-vendor <v> Return parts that have a vendor of <v>.\n");
	printf("  -j,--json             Print information as JSON.\n");
}

void print_entry(struct invent_entry *entry)
{
	printf("%s: %s%s$%d.%02d - %.2f on hand\n", entry->part_number, entry->bin_location,
		entry->bin_location[0] ? ", " : "", entry->price / 100, entry->price % 100, entry->on_hand);
	if(strlen(entry->desc) > 0)
		printf("%s\n", entry->desc);
	if(strlen(entry->ext_desc) >0)
		printf("%s\n", entry->ext_desc);
	printf("\n");
}

void print_value_null(char *value)
{
	if(value && strlen(value) > 0)
		printf("\"%s\"", value);
	else
		printf("null");
}

void print_double_two(double d)
{
	if(d == 0.0 || d == 1.0 || d == 2.0 || d == 3.0 || d == 4.0 ||
		d == 5.0 || d == 6.0 || d == 7.0 || d == 8.0 || d == 9.0)
	{
		putchar('0' + (int)d);
	}
/*	if(d == 0.0)
		putchar('0');
	else if(d == 1.0)
		putchar('1');
	else if(d == 2.0)
		putchar('2');
	else if(d == 3.0)
		putchar('3');*/
//	if(d - (int)d == 0)
//		printf("%d", (int)d);
	else
		printf("%0.2f", d);
}

void print_entry_json(struct invent_entry *entry)
{
	puts("{");
	puts("\"partnumber\":"); puts(json_stringify(entry->part_number));
	printf(",\"price\":%d", (int)entry->price);
	puts(",\"onhand\":"); print_double_two(entry->on_hand);
	puts(",\"bin\":"); puts(json_stringify(entry->bin_location));
	puts(",\"binalt1\":"); puts(json_stringify(entry->bin_alt1));
	puts(",\"binalt2\":"); puts(json_stringify(entry->bin_alt2));
	puts(",\"desc\":"); puts(json_stringify(entry->desc));
	puts(",\"extdesc\":"); puts(json_stringify(entry->ext_desc));
	puts(",\"note1\":"); puts(json_stringify(entry->note1));
	puts(",\"note2\":"); puts(json_stringify(entry->note2));
	puts(",\"histmonth\":[");
	for(int i = 0; i < MONTH_HIST_LEN - 1; i++) // Subtract 1 so we have an element left...
	{
		print_double_two(entry->month_history[i].value);
		putchar(',');
	}
	print_double_two(entry->month_history[MONTH_HIST_LEN-1].value);
	putchar(']'); // ...which we print without a comma
	puts(",\"histyear\":[");
	for(int i = 0; i < YEAR_HIST_LEN - 1; i++)
	{
		print_double_two(entry->year_history[i].value);
		putchar(',');
	}
	print_double_two(entry->year_history[YEAR_HIST_LEN-1].value);
	putchar(']');
	puts("}\n");
}

int main(int argc, char *argv[])
{
	char *filename = "INVENT";
	char *to_find = NULL;
	char *pattern_to_find = NULL;
	int search_desc=0,search_ext_desc=0,search_part_number=1;
	struct option long_options[] =
	{
		{"help",	no_argument,		0,	'h'},
		{"find",	required_argument,	0,	'f'},
		{"all",		no_argument,		0,	'a'},
		{"regex",	required_argument,	0,	'r'},
		{"json",	no_argument,		0,	'j'},
		{"search-desc",no_argument,		&search_desc, 1},
		{"search-extdesc",no_argument,	&search_ext_desc, 1},
		{"match-vendor",required_argument, 0, 'v'},
		{0,0,0,0}
	};
	int option_index = 0;
	int c;
	int print_all_flag = 0, print_as_json = 0;
	pcre2_code *regex = NULL;
	int errorcode = 0;
	PCRE2_SIZE erroroffset = 0;
	int ret = 0;
	bool found_some_results = false;
	char *match_vendor = NULL;
	char part_numbers[MAX_PART_NUMBERS][sizeof(((struct invent_entry*)0)->part_number)] = {0};
	int part_numbers_idx = 0;
	while(1)
	{
		c = getopt_long(argc, argv, "jhf:ar:", long_options, &option_index);
		if(c == -1)
			break;
		switch(c)
		{
			case 'h':
				print_usage(argv[0]);
				return 0;
			case 'f':
				to_find = optarg;
				normalize_part_number(to_find);
				strcpy(part_numbers[part_numbers_idx++], to_find);
				break;
			case 'j':
				print_as_json = 1;
				break;
			case 'a':
				print_all_flag = 1;
				break;
			case 'r':
				pattern_to_find = optarg;
				break;
			case 'v':
				match_vendor = optarg;
				break;
		}
	}
	if(optind < argc)
	{
		filename = argv[optind++];
	}
	
	// If >1 files supplied, print usage information.
	// *OR* if we have no pattern or part number to find, and we aren't printing all
	if(optind > argc || (!pattern_to_find && !to_find && !print_all_flag && !match_vendor))
	{
		print_usage(argv[0]);
		return 0;
	}
	
	if(pattern_to_find)
	{
		// If we have a regex, compile it and check for errors
		// case-insensitive
		regex = pcre2_compile((PCRE2_SPTR)pattern_to_find, PCRE2_ZERO_TERMINATED, PCRE2_CASELESS, &errorcode, &erroroffset, NULL);
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
	fseek(fp, INVENT_FIRST_ENTRY_OFFSET, SEEK_SET);
	if(ftell(fp) != INVENT_FIRST_ENTRY_OFFSET)
	{
		printf("fseek failed: %s\n", strerror(errno));
		return 1;
	}
	if(print_as_json)
	{
		printf("[");
	}
	while(1)
	{
		struct invent_entry entry = {0};
		if(!fread(record, INVENT_ENTRY_SIZE, 1, fp))
		{
			if(feof(fp))
				break;
			printf("fgets failed: %s\n", strerror(errno));
			return 1;
		}
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
		#define entry_copy(field, record) do{memcpy((field), record, sizeof(field)-1);(field)[sizeof(field)-1]='\0';}while(0)
		#define entry_copy_nonull(field, record) do{memcpy((field), record, sizeof(field));}while(0)
		//record+offset in bytes
		entry_copy(entry.part_number, record); //20?
		entry_copy(entry.vendor, record+30); //3
		entry_copy(entry.mfrid, record+36); //5
		entry_copy(entry.desc, record+58); //10?
		entry_copy(entry.bin_location, record+73); //12?
		entry_copy(entry.bin_alt1, record+85); //12
		entry_copy(entry.bin_alt2, record+97); //12
		entry_copy(entry.entry_date, record+148); //8
		entry_copy(entry.ext_desc, record+1556); //60
		// If you've got a wierd bug with the month_history/year_history arrays getting corrupted, check the comment at the definition of struct padded_double. Packed structs aren't portable.
		entry_copy_nonull(entry.month_history, record+298);
		entry_copy_nonull(entry.year_history, record+970);
		memcpy(&entry.on_hand, record+1112, sizeof(entry.on_hand));
		double dprice = 0, dcost = 0;
		memcpy(&dprice, record+188, sizeof(dprice));
		memcpy(&dcost, record+180, sizeof(dcost));
		entry.price = (int)nearbyint(dprice);
		entry.cost = (int)nearbyint(dcost);
		entry.has_comments = false;
		if(record[1616] == 'Y')
		{
			entry.has_comments = true;
		}

		bool i_match = false;
		if(print_all_flag)
		{
			i_match = true;
		}
		else if(match_vendor && !strncmp(match_vendor, entry.vendor, 3))
		{
			i_match = true;
		}
		else if(to_find)
		{
			char tmp[sizeof(((struct invent_entry*)0)->part_number)];
			strcpy(tmp, entry.part_number);
			normalize_part_number(tmp);
			for(int i = 0; part_numbers[i][0] != 0; i++)
			{
				if(!strcasecmp(tmp, part_numbers[i]))
					i_match = true;
			}
		}
		else if(pattern_to_find)
		{
			pcre2_match_data *matchdata;
			matchdata = pcre2_match_data_create_from_pattern(regex, NULL);
			if(search_part_number)
				ret = pcre2_match(regex, (PCRE2_SPTR)entry.part_number, PCRE2_ZERO_TERMINATED, 0, 0, matchdata, NULL);
			if(search_desc && ret < 0)
				ret = pcre2_match(regex, (PCRE2_SPTR)entry.desc, PCRE2_ZERO_TERMINATED, 0, 0, matchdata, NULL);
			if(search_ext_desc && ret < 0)
				ret = pcre2_match(regex, (PCRE2_SPTR)entry.ext_desc, PCRE2_ZERO_TERMINATED, 0, 0, matchdata, NULL);
			if(ret > 0)
				i_match = true;
		}
		if(i_match)
		{
			if(print_as_json) // If JSON output...
			{
				if(found_some_results) // ... and this isn't the first result,
					printf(","); // do some comma separating of records
				print_entry_json(&entry);
			}
			else
			{
				print_entry(&entry);
			}
			found_some_results = true; // Don't set this to true too soon, or we will print a leading comma in our JSON array!!
		}
	}
	if(print_as_json)
		printf("]\n");
	else if(!found_some_results)
		printf("No matches found.\n");
	fclose(fp);
	return 0;
}
