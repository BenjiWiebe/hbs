#ifndef _INVENT_ENTRY_INCLUDED_
#define _INVENT_ENTRY_INCLUDED_

#include <stdbool.h>
#include <stdint.h>

#define INVENT_FIRST_ENTRY_OFFSET	2048
#define INVENT_ENTRY_SIZE			2048

#define MONTH_HIST_LEN	48
#define YEAR_HIST_LEN	9

#define INVENT_TYPE_MEMO	'9'
#define INVENT_TYPE_USED	'7'
#define INVENT_TYPE_CORE	'5'
#define INVENT_TYPE_REMAN	'3'
#define INVENT_TYPE_NEW		'1'

// If something goes wrong on an architecture other than x86/x86_64 or with a non-GCC compiler, refactor the code to just copy the double in a loop rather than doing fancy un-portable packed struct memcpy's.
struct padded_double {
	unsigned char padding[6];
	double value;
} __attribute__((packed));


struct invent_entry {
	char part_number[21]; // length 20, offset 0
	char status[2]; // length 1, offset 52. A=Active, O=Obsolete
	uint8_t type; // length 1, offset 35. INVENT_TYPE_MEMO/USED/NEW/etc
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
	char new_partno[21]; // length 20, offset 1176
	char new_vendor[4]; // length 3, offset 1206
	uint8_t new_type; // length 1, offset 1211
	char old_partno[21]; // length 20, offset 1220
	char old_vendor[4]; // length 3, offset 1250
	uint8_t old_type; // length 1, offset 1255
	char core_partno[21]; // length 20, offset 1264
	char core_vendor[4]; // length 3, offset 1294
	uint8_t core_type; // length 1, offset 1299
	char reman_partno[21]; // length 20, offset 1308
	char reman_vendor[4]; // length 3, offset 1338
	uint8_t reman_type; // length 1, offset 1343
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

#endif
