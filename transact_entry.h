#ifndef _TRANSACT_ENTRY_INCLUDED_
#define _TRANSACT_ENTRY_INCLUDED_

#include <inttypes.h>

#define TYPE_MEMO	'9'
#define TYPE_USED	'7'
#define TYPE_CORE	'5'
#define TYPE_REMAN	'3'
#define TYPE_NEW	'1'

#define TRANSACT_RECORD_SIZE	128
#define TRANSACT_FILE_SKIP		128


struct transact_entry {
	char part_number[21]; // length 20, offset 0
	char vendor[4]; // length 3, offset 30
	uint8_t type; // length 1, offset 35
	uint32_t when; // length 4, offset 44
	char desc[13]; // length 12, offset 50
	double change; // length 8, offset 62
	double newqty; // length 8, offset 70
	char customer[21]; // length 20, offset 78
	char customernumber[6]; // length 5, offset 116
};

#endif
