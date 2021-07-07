#ifndef _UPDATE_DB_INCLUDED_
#define _UPDATE_DB_INCLUDED_

#include "invent_entry.h"

int update_db_init(void);
int update_db_record(struct invent_entry*);
int update_db_finalize(void);

#endif
