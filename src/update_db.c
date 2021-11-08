#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "update_db.h"

static sqlite3 *db;
static sqlite3_stmt *insert_stmt, *insert2_stmt, *select_stmt;

static char *sql_create_table_inventory_parts = " \
	CREATE TABLE IF NOT EXISTS inventory_parts ( \
	id INTEGER PRIMARY KEY AUTOINCREMENT, \
	partno TEXT NOT NULL, \
	vendor INTEGER NOT NULL, \
	type INTEGER NOT NULL, \
	UNIQUE(partno,vendor,type) \
) \
";

static char *sql_create_table_inventory_changes = " \
	CREATE TABLE IF NOT EXISTS inventory_changes ( \
	id INTEGER PRIMARY KEY AUTOINCREMENT, \
	partno TEXT NOT NULL, \
	vendor INTEGER NOT NULL, \
	onhand INTEGER NOT NULL, \
	type INTEGER NOT NULL, \
	t TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL, \
	UNIQUE(partno,vendor,type,t) \
	) \
";

static char *sql_create_table_inventory_current = " \
	CREATE TABLE IF NOT EXISTS inventory_current ( \
	id INTEGER PRIMARY KEY AUTOINCREMENT, \
	partno TEXT NOT NULL, \
	vendor INTEGER NOT NULL, \
	onhand INTEGER NOT NULL, \
	type INTEGER NOT NULL, \
	UNIQUE(partno,vendor,type) \
	) \
";

static char *sql_create_index = "CREATE INDEX IF NOT EXISTS idx_partid \
	ON inventory_changes(partno);";
	
static char *sql_insert_record = "INSERT OR REPLACE INTO inventory_changes (partno, vendor, onhand, type) VALUES (?,?,?)";
static char *sql_insert_record_two = "INSERT INTO inventory_current (partno, vendor, onhand, type) VALUES (?,?,?)";

#define sqlcheckerror(ret, msg, db)		do{if(ret!=SQLITE_OK){fprintf(stderr,msg  ": %s\n",sqlite3_errmsg(db));return ret;}}while(0)

static char *sql_select = "SELECT * FROM inventory_current ORDER BY partno ASC, vendor ASC";

typedef struct {
	char part_number[21];
	int vendor;
	double on_hand;
} pn_on_hand_t;
static pn_on_hand_t *on_hand_values;

static size_t on_hand_values_alloc_length = 10000, on_hand_values_length;

// Returns 0 on success
int update_db_init(void)
{
	// Initialize memory here??
	on_hand_values = malloc(on_hand_values_alloc_length*sizeof(pn_on_hand_t));
	if(on_hand_values == NULL)
	{
		fprintf(stderr, "Memory allocation failed: %s\n", strerror(errno)); 
		return 1;
	}
	

	// Also open the DB, and get the list of current on-hand numbers.
	// Get the list sorted, so we can check against it with binary searching.

	// Open the SQLite database
	int ret = sqlite3_open("invent_history.db", &db);
	sqlcheckerror(ret, "Opening database failed", db);


	// Create our tables (if not exists), and indices
	ret = sqlite3_exec(db, sql_create_table_inventory_changes, NULL, NULL, NULL);
	sqlcheckerror(ret, "Creating table failed", db);
	ret = sqlite3_exec(db, sql_create_table_inventory_current, NULL, NULL, NULL);
	sqlcheckerror(ret, "Creating table failed", db);
	ret = sqlite3_exec(db, sql_create_index, NULL, NULL, NULL);
	sqlcheckerror(ret, "Creating index failed", db);

		
	// Prepare all the statements
	ret = sqlite3_prepare_v2(db, sql_select, -1, &select_stmt, NULL);
	sqlcheckerror(ret, "Preparing statement failed", db);
	ret = sqlite3_prepare_v2(db, sql_insert_record, -1, &insert_stmt, NULL);
	sqlcheckerror(ret, "Preparing statement failed", db);
	ret = sqlite3_prepare_v2(db, sql_insert_record_two, -1, &insert2_stmt, NULL);
	sqlcheckerror(ret, "Preparing statement failed", db);

	// Get the list of current on hand values
	while((ret = sqlite3_step(select_stmt)) == SQLITE_ROW)
	{
		size_t pn_len = sizeof(((pn_on_hand_t*)0)->part_number);
		// NULL-terminate and don't copy too far, just in case the SQL db returns unusually long part numbers
		on_hand_values[on_hand_values_length].part_number[pn_len-1] = 0;
		strncpy(on_hand_values[on_hand_values_length].part_number, sqlite3_column_text(select_stmt, 1), pn_len);
		on_hand_values[on_hand_values_length].vendor = sqlite3_column_int(select_stmt, 2);
		on_hand_values[on_hand_values_length].on_hand = sqlite3_column_double(select_stmt, 3);
		on_hand_values_length++;
		if(on_hand_values_length == on_hand_values_alloc_length)
		{
			//realloc 2x
			void *newbuf = realloc(on_hand_values, on_hand_values_alloc_length * 2 * sizeof(pn_on_hand_t));
			if(!newbuf)
			{
				fprintf(stderr, "Failed to resize memory: %s\n", strerror(errno));
				return 1;
			}
			on_hand_values = newbuf;
			on_hand_values_alloc_length *= 2;
		}
	}
	if(ret != SQLITE_DONE)
	{
		sqlcheckerror(ret, "Selecting data failed", db);
	}

	// Begin the transaction
	ret = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
	sqlcheckerror(ret, "SQL Error", db);

	return 0;
}

static int compare_entry(const void *_a, const void *_b)
{
	const pn_on_hand_t *a = _a, *b = _b;
	int r = strcmp(a->part_number, b->part_number);
	if(r == 0)
		return a->vendor - b->vendor;
	else
		return r;
}

int update_db_record(struct invent_entry* entry)
{
	// Find *entry in the in-memory list, and see if the on-hand changed.
	// If it did, INSERT the change and UPDATE the current amounts table.

	// do a binary search of on_hand_values
	pn_on_hand_t key = {0};
	strncpy(key.part_number, entry->part_number, sizeof(key.part_number));
	pn_on_hand_t *result = bsearch(&key, on_hand_values, on_hand_values_length, sizeof(pn_on_hand_t), compare_entry);
	if(!result)
		printf("didnt find %s\n", entry->part_number);
	// We *can* compare the doubles directly, *as long as* we insert the double into the DB as is
	// If the part hasn't been found, we need to insert it regardless of any change in on hand values.
	if(result != NULL && result->on_hand == entry->on_hand)
		return 0;

	printf("updating %s\n", entry->part_number);

	//ELSE
	//sqlite3: Record the on hand change. The timestamp is auto-filled by the DB.
	// Don't bother with error codes returned by sqlite3_reset...
	// It returns the same error code that the previous _step returned, which
	// we *do* check.
	sqlite3_reset(insert_stmt);
	sqlite3_reset(insert2_stmt);

	int ret = sqlite3_bind_text(insert_stmt, 1, entry->part_number, -1, NULL);
	sqlcheckerror(ret, "Binding failed", db);
	ret = sqlite3_bind_text(insert_stmt, 2, entry->vendor, -1, NULL);
	sqlcheckerror(ret, "Binding failed", db);
	ret = sqlite3_bind_double(insert_stmt, 3, entry->on_hand);
	sqlcheckerror(ret, "Binding failed", db);
	ret = sqlite3_bind_int(insert_stmt, 4, entry->type);
	sqlcheckerror(ret, "Binding failed", db);
	ret = sqlite3_bind_text(insert2_stmt, 1, entry->part_number, -1, NULL);
	sqlcheckerror(ret, "Binding failed", db);
	ret = sqlite3_bind_text(insert2_stmt, 2, entry->vendor, -1, NULL);
	sqlcheckerror(ret, "Binding failed", db);
	ret = sqlite3_bind_double(insert2_stmt, 3, entry->on_hand);
	sqlcheckerror(ret, "Binding failed", db);
	ret = sqlite3_bind_int(insert2_stmt, 4, entry->type);
	sqlcheckerror(ret, "Binding failed", db);

	//sqlite3: Update the current on hand number. Insert if there isn't one!


	ret = sqlite3_step(insert_stmt);
	if(ret != SQLITE_DONE)
	{
		printf("stepping failed for %s:%s\n", entry->part_number, entry->vendor);
		sqlcheckerror(ret, "Stepping failed", db);
	}
	ret = sqlite3_step(insert2_stmt);
	if(ret != SQLITE_DONE)
	{
		printf("stepping2 failed for %s:%s\n", entry->part_number, entry->vendor);
		sqlcheckerror(ret, "Stepping failed", db);
	}
	return 0;
}

int update_db_finalize(void)
{
	// Commit our changes to the database
	int ret = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
	sqlcheckerror(ret, "SQL Error", db);
	sqlite3_finalize(insert_stmt);
	sqlite3_finalize(select_stmt);
	sqlite3_close(db);
}
