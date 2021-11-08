#include <stdio.h>
#include <string.h>
#include "transact_entry.h"

// 'from' needs to point to a chunk of data from the flat file database TRANSACT, n bytes in size
// 'to' points to a chunk of memory where we can store the transact_entry data
void populate_transact_from_record(struct transact_entry *to, void *from)
{
	size_t s;

	s = sizeof(to->part_number)-1;
	memcpy(to->part_number, from, s);
	to->part_number[s] = 0;

	s = sizeof(to->vendor)-1;
	memcpy(to->vendor, from+30, s);
	to->vendor[s] = 0;

	s = sizeof(to->desc)-1;
	memcpy(to->desc, from+50, s);
	to->desc[s] = 0;

	s = sizeof(to->customer)-1;
	memcpy(to->customer, from+78, s);
	to->customer[s] = 0;

	s = sizeof(to->customernumber)-1;
	memcpy(to->customernumber, from+116, s);
	to->customernumber[s] = 0;

	to->type = *(char*)(from+35);

	memcpy(&to->when, from+44, 4);

	memcpy(&to->change, from+62, sizeof(double));

	memcpy(&to->newqty, from+70, sizeof(double));
}

void insert_record_in_db(sqlite3 *db, struct transact_entry *e)
{
	const char *sql = "INSERT INTO transact_records (partno, vendor, desc, customer, customernumber
}

int main()
{
	FILE *fp = fopen("TRANSACT", "r");
	char buf[TRANSACT_RECORD_SIZE];
	fseek(fp, TRANSACT_FILE_SKIP, SEEK_SET);
	while(fread(buf, TRANSACT_RECORD_SIZE, 1, fp) > 0)
	{
		struct transact_entry e;
		populate_transact_from_record(&e, buf);
		printf("%s %s\n", e.part_number, e.customer);
	}
	fclose(fp);
}
