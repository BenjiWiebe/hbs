#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <getopt.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "normalize.h"
#include "json_stringify.h"
#include "invent_entry.h"
#include "update_db.h"
#include "print_entry_json.h"

#define INVDESC_FIRST_ENTRY_OFFSET	512
#define INVDESC_ENTRY_SIZE		512

// Maximum number of -f <partno> to store from the command line
#define MAX_PART_NUMBERS	64


char record[INVENT_ENTRY_SIZE];

void print_usage(char *argv0)
{
	printf("Usage: %s [filename]\n", argv0);
	printf(" Matching options\n");
	printf("  -a,--match-all         Match all records.\n");
	printf("  -r,--match-regex       Match records against regex.\n");
	printf("     --match-regex-all   Match records against regex, searching all relevant fields.\n");
	printf("     --match-partno      Match records with matching part number.\n");
	printf("     --match-vendor      Match records by vendor number.\n");
	printf("     --match-bin         Match records by bin location.\n");
	printf("\n Action options\n");
	printf("     --update-qty-db     Update the record(s) in the quantity database.\n");
	printf("  -p,--print (text|json) Print the record(s) as JSON or plain text.\n");
	printf("     --print-history     Include the history when printing JSON.\n");
	printf("\n Other\n");
	printf("  -t,--type <type>       The type of the database. Currently only INVENT is supported.\n");
	printf("  -h,--help              Print this message.\n");
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

int main(int argc, char *argv[])
{
	char *filename;
	//char *to_find = NULL;
	//char *pattern_to_find = NULL;
	//int search_desc=0,search_ext_desc=0,search_part_number=1;
	struct option long_options[] = 
	{
		{"match-all",		no_argument,		0,	'a'},
		{"match-regex",		required_argument,	0,	'r'},
		{"match-regex-all",	required_argument,	0,	2},
		{"match-partno",	required_argument,	0,	3},
		{"match-vendor",	required_argument,	0,	4},
		{"match-bin",		required_argument,	0,	5},
		{"update-qty-db",	no_argument,		0,	6},
		{"print",			required_argument,	0,	'p'},
		{"print-history",	no_argument,		0,	7},
		{"type",			required_argument,	0,	't'},
		{"help",			no_argument,		0,	'h'},
		{0,0,0,0}
	};
	int option_index = 0;
	int c;
	int print_all_flag = 0, print_as_json = 0, do_update_db = 0;
	int errorcode = 0;
	PCRE2_SIZE erroroffset = 0;
	int ret = 0;
	bool is_first_match = true;
	char *match_vendor = NULL;
	char part_numbers[MAX_PART_NUMBERS][sizeof(((struct invent_entry*)0)->part_number)] = {0};
	int part_numbers_idx = 0;
	
	struct {
		bool match_all;
		char *pattern_partno;
		char *pattern_all;
		char *fuzzy_partno;
		char *bin;
		char *vendor;
		pcre2_code *regex_all;
		pcre2_code *regex_partno;
		pcre2_match_data *matchdata_all;
		pcre2_match_data *matchdata_partno;
	} match_options = {0};

	struct {
		bool print;
		bool include_history;
		enum {PRINT_JSON, PRINT_PLAINTEXT} print_as;
		bool update_qty_db;
	} action_options = {0};
	while(1)
	{
		c = getopt_long(argc, argv, "ahp:r:t:", long_options, &option_index);
		if(c == -1)
			break;
		switch(c)
		{
			case 'h':
				print_usage(argv[0]);
				return 0;

			case 'p':
				action_options.print = true;
				if(!strcmp(optarg, "json"))
				{
					action_options.print_as = PRINT_JSON;
				}
				else if(!strcmp(optarg, "text"))
				{
					action_options.print_as = PRINT_PLAINTEXT;
				}
				else
				{
					fprintf(stderr, "Unknown print type %s.\n", optarg);
					return 1;
				}
				break;

			case 'a': match_options.match_all = true; break;
			case 'r': match_options.pattern_partno = optarg; break;
			case 2: match_options.pattern_all = optarg; break;
			case 3: match_options.fuzzy_partno = optarg; normalize_part_number(match_options.fuzzy_partno); break;
			case 4: match_options.vendor = optarg; break;
			case 5: match_options.bin = optarg; break;
			case 6: action_options.update_qty_db = true; break;
			case 7: action_options.include_history = true; break;
			case 't': fprintf(stderr, "--type not supported yet.\n"); break;
		}
	}



	// Figure out what file we are working on.
	if(optind < argc)
	{
		filename = argv[optind++];
	}
	else
	{
		filename = "INVENT";
	}

	// Do we have leftover arguments? That's not OK!
	if(optind < argc)
	{
		print_usage(argv[0]);
		return 1;
	}

	// Do we have _some_ action specified?
	if(!action_options.print && !action_options.update_qty_db)
	{
		fprintf(stderr, "No actions specified.\n");
		print_usage(argv[0]);
		return 1;
	}

	// Compile our regexes and report on errors
	if(match_options.pattern_partno)
	{
		// If we have a regex, compile it and check for errors
		// case-insensitive
		match_options.regex_partno = pcre2_compile((PCRE2_SPTR)match_options.pattern_partno, PCRE2_ZERO_TERMINATED, PCRE2_CASELESS, &errorcode, &erroroffset, NULL);
		if(match_options.regex_partno == NULL)
		{
			PCRE2_UCHAR buffer[256];
			pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
			fprintf(stderr, "Regex error at offset %d: %s\n", (int)erroroffset, buffer);
			return 1;
		}

		match_options.matchdata_partno = pcre2_match_data_create_from_pattern(match_options.regex_partno, NULL);
	}
	if(match_options.pattern_all)
	{
		match_options.regex_all = pcre2_compile((PCRE2_SPTR)match_options.pattern_all, PCRE2_ZERO_TERMINATED, PCRE2_CASELESS, &errorcode, &erroroffset, NULL);
		if(match_options.regex_all == NULL)
		{
			PCRE2_UCHAR buffer[256];
			pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
			fprintf(stderr, "Regex error at offset %d: %s\n", (int)erroroffset, buffer);
			return 1;
		}

		match_options.matchdata_all = pcre2_match_data_create_from_pattern(match_options.regex_all, NULL);
	}


	// first entry at 2048 bytes
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL)
	{
		fprintf(stderr, "Error opening %s: %s\n", filename, strerror(errno));
		return 1;
	}
	fseek(fp, INVENT_FIRST_ENTRY_OFFSET, SEEK_SET);
	if(ftell(fp) != INVENT_FIRST_ENTRY_OFFSET)
	{
		fprintf(stderr, "fseek failed: %s\n", strerror(errno));
		return 1;
	}

	if(action_options.update_qty_db)
	{
		if(update_db_init() != 0)
		{
			fprintf(stderr, "Failed to initialize database.\n");
			exit(EXIT_FAILURE);
		}
	}

	size_t results_count = 0; // Keep track of how many matches we found
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

		// Check for deleted(?) records
		if(record[0]=='\xFF')
			continue;
		
		// Check for blank(?) records
		if(record[0]=='\x00')
			continue;


		// *** We've got a record. Let's copy it to a nice little struct. ***

		// entry_copy copies sizeof(field) bytes and ensures that the last byte of the array is NULL/0.
		// entry_copy_nonull just copies sizeof(field) bytes.
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
		entry_copy(entry.new_partno, record+1176);
		entry_copy(entry.new_vendor, record+1206);
		entry_copy(entry.old_partno, record+1220);
		entry_copy(entry.old_vendor, record+1250);
		entry_copy(entry.core_partno, record+1264);
		entry_copy(entry.core_vendor, record+1294);
		entry_copy(entry.reman_partno, record+1308);
		entry_copy(entry.reman_vendor, record+1338);
		entry.type = *(record+35);
		entry.new_type = *(record+1211);
		entry.old_type = *(record+1255);
		entry.core_type = *(record+1299);
		entry.reman_type = *(record+1343);
		// If you've got a weird bug with the month_history/year_history arrays getting corrupted, check the comment at the definition of struct padded_double. Packed structs aren't portable.
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


		// *** At this point, we have the record from the database safely read into the 'entry' variable. ***
		// This section does various matching logic or other handling of the record.

		// This variable is set to true if the current record matches the search parameters.
		// If it's true, we need to run the specified actions on this record, whatever they may be.
		bool i_match = false;
		
		if(match_options.match_all)
		{
			i_match = true;
		}
		else if(match_options.vendor && !strncmp(match_options.vendor, entry.vendor, 3))
		{
			i_match = true;
		}
		else if(match_options.fuzzy_partno)
		{
			char tmp[sizeof(((struct invent_entry*)0)->part_number)];
			strcpy(tmp, entry.part_number);
			normalize_part_number(tmp);
			if(!strcasecmp(tmp, match_options.fuzzy_partno))
				i_match = true;
		}
		else if(match_options.regex_partno)
		{
			ret = pcre2_match(match_options.regex_partno, (PCRE2_SPTR)entry.part_number, PCRE2_ZERO_TERMINATED, 0, 0, match_options.matchdata_partno, NULL);
			if(ret > 0)
				i_match = true;
		}
		else if(match_options.regex_all)
		{
			ret = pcre2_match(match_options.regex_all, (PCRE2_SPTR)entry.part_number, PCRE2_ZERO_TERMINATED, 0, 0, match_options.matchdata_all, NULL);
			if(ret < 0)
				ret = pcre2_match(match_options.regex_all, (PCRE2_SPTR)entry.desc, PCRE2_ZERO_TERMINATED, 0, 0, match_options.matchdata_all, NULL);
			if(ret < 0)
				ret = pcre2_match(match_options.regex_all, (PCRE2_SPTR)entry.ext_desc, PCRE2_ZERO_TERMINATED, 0, 0, match_options.matchdata_all, NULL);
			if(ret > 0)
				i_match = true;
		}
		
		if(!i_match) // we don't want this record, let's go to the next one
			continue;

		// At this point, our record has matched our search terms.
		// Let's do whatever actions we need to on it.

		results_count++; // keep track of how many matches we've found

		if(action_options.update_qty_db)
			update_db_record(&entry);

		if(action_options.print)
		{
			if(action_options.print_as == PRINT_JSON) // If JSON output...
			{
				// If it's the first result, print the opening bracket. Otherwise, print the separating comma.
				if(is_first_match)
				{
					putchar('[');
				}
				else // ... and this isn't the first result,
				{
					putchar(','); // do some comma separating of records
				}
				print_entry_json(&entry, action_options.include_history);
			}
			else
			{
				print_entry(&entry);
			}
		}

		is_first_match = false; // keep track of first record for printing commas in the JSON array output

	} // end per-part while loop

	// Do various cleanup and and things and stuff, etc.
	pcre2_match_data_free(match_options.matchdata_partno);
	pcre2_match_data_free(match_options.matchdata_all);
	pcre2_code_free(match_options.regex_partno);
	pcre2_code_free(match_options.regex_all);
	fclose(fp);
	if(do_update_db)
		update_db_finalize();
	if(action_options.print)
	{
		if(action_options.print_as == PRINT_JSON)
		{
			if(results_count == 0) // if we have no results, there won't be an opening bracket
				putchar('[');
			printf("]\n");
		}
		else if(results_count == 0)
		{
			printf("No matches found.\n");
		}
	}
	return 0;
}
